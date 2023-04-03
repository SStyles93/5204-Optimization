#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <cstdint>
#include <chrono>
#include <vector>

#define GLM_FORCE_INLINE
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <benchmark/benchmark.h>
#include <tiny_obj_loader.h>
#include <png.h>
#include <stb_image.h>

#if TRACY_ENABLE
#include "tracy/Tracy.hpp"
#endif // TRACY_ENABLE


// Silence macro redefinition warnings
#undef TO_RASTER
// Transform a given vertex in clip-space [-w,w] to raster-space [0, {w|h}]
#define TO_RASTER(v) glm::vec4((g_scWidth * (v.x + v.w) / 2), (g_scHeight * (v.w - v.y) / 2), v.z, v.w)

static constexpr auto g_scWidth = 1280u;
static constexpr auto g_scHeight = 720u;
static constexpr glm::vec3 up(0, 1, 0);
static constexpr glm::mat4 indentity(1.f);


// Used for texture mapping
struct Texture
{
    stbi_uc* data = nullptr;
    int32_t     width = -1;
    int32_t     height = -1;
    int32_t     numChannels = -1;
};

// Vertex data to be fed into each VS invocation as input
struct VertexInput
{
    glm::vec3   pos;
    glm::vec3   normal;
    glm::vec2   texCoords;
};

// Vertex Shader payload, which will be passed to each FS invocation as input
struct FragmentInput
{
    glm::vec3   normal;
    glm::vec2   texCoords;
};

// Indexed mesh
struct Mesh
{
    // Offset into the global index buffer
    uint32_t    idxOffset = 0u;

    // How many indices this mesh contains. Number of triangles therefore equals (m_IdxCount / 3)
    uint32_t    idxCount = 0u;

    // Texture map from material
    std::string diffuseTexName;
};

void DrawIndexed(std::vector<glm::vec3>& frameBuffer, std::vector<float>& depthBuffer, std::vector<VertexInput>& vertexBuffer, std::vector<uint32_t>& indexBuffer, Mesh& mesh, glm::mat4& MVP, Texture* pTexture);

void OutputFrameForScene(const std::vector<glm::vec3>& frameBuffer, const char* filename)
{
    assert(frameBuffer.size() >= (g_scWidth * g_scHeight));

    FILE* pFile = nullptr;
    fopen_s(&pFile, filename, "w");
    fprintf(pFile, "P3\n%d %d\n%d\n ", g_scWidth, g_scHeight, 255);
    for (auto i = 0; i < g_scWidth * g_scHeight; ++i)
    {
        // Write out color values clamped to [0, 255] 
        uint32_t r = static_cast<uint32_t>(255 * glm::clamp(frameBuffer[i].r, 0.0f, 1.0f));
        uint32_t g = static_cast<uint32_t>(255 * glm::clamp(frameBuffer[i].g, 0.0f, 1.0f));
        uint32_t b = static_cast<uint32_t>(255 * glm::clamp(frameBuffer[i].b, 0.0f, 1.0f));
        fprintf(pFile, "%d %d %d ", r, g, b);
    }
    fclose(pFile);
}

void OutputPngForScene(const std::vector<glm::vec3>& frameBuffer, const char* filename)
{
    assert(frameBuffer.size() >= (g_scWidth * g_scHeight));

    FILE* pFile = nullptr;
    fopen_s(&pFile, filename, "wb"); // Use binary mode for writing
    assert(pFile != nullptr);

    // Initialize the PNG writer
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    assert(png_ptr != nullptr);

    png_infop info_ptr = png_create_info_struct(png_ptr);
    assert(info_ptr != nullptr);

    png_init_io(png_ptr, pFile);

    // Write the PNG header
    png_set_IHDR(png_ptr, info_ptr, g_scWidth, g_scHeight, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);

    // Allocate a row buffer for writing the image data
    png_bytep row = (png_bytep)malloc(g_scWidth * 3);

    // Write the image data, row by row
    for (auto y = 0; y < g_scHeight; ++y)
    {
        for (auto x = 0; x < g_scWidth; ++x)
        {
            // Get the pixel color values, clamped to [0, 255]
            uint32_t r = static_cast<uint32_t>(255 * glm::clamp(frameBuffer[y * g_scWidth + x].r, 0.0f, 1.0f));
            uint32_t g = static_cast<uint32_t>(255 * glm::clamp(frameBuffer[y * g_scWidth + x].g, 0.0f, 1.0f));
            uint32_t b = static_cast<uint32_t>(255 * glm::clamp(frameBuffer[y * g_scWidth + x].b, 0.0f, 1.0f));

            // Write the pixel color values to the row buffer
            row[x * 3 + 0] = static_cast<png_byte>(r);
            row[x * 3 + 1] = static_cast<png_byte>(g);
            row[x * 3 + 2] = static_cast<png_byte>(b);
        }

        // Write the row buffer to the PNG file
        png_write_row(png_ptr, row);
    }

    // Clean up
    free(row);
    png_write_end(png_ptr, nullptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(pFile);
}

void InitializeSceneObjects(const char* fileName, std::vector<Mesh>& meshBuffer, std::vector<VertexInput>& vertexBuffer, std::vector<uint32_t>& indexBuffer, std::map<std::string, Texture*>& textures)
{
    tinyobj::attrib_t attribs;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err = "";

    bool ret = tinyobj::LoadObj(&attribs, &shapes, &materials, nullptr, &err, fileName, "../assets/", true /*triangulate*/, true /*default_vcols_fallback*/);
    if (ret)
    {
        // Process materials to load images
        {
            for (unsigned i = 0; i < materials.size(); i++)
            {
                const tinyobj::material_t& m = materials[i];

                std::string diffuseTexName = m.diffuse_texname;
                assert(!diffuseTexName.empty() && "Mesh missing texture!");

                if (textures.find(diffuseTexName) == textures.end())
                {
                    Texture* pAlbedo = new Texture();

                    pAlbedo->data = stbi_load(("../assets/" + diffuseTexName).c_str(), &pAlbedo->width, &pAlbedo->height, &pAlbedo->numChannels, 0);
                    assert(pAlbedo->data != nullptr && "Failed to load image!");

                    textures[diffuseTexName] = pAlbedo;
                }
            }
        }

        // Process vertices
        {
            // POD of indices of vertex data provided by tinyobjloader, used to map unique vertex data to indexed primitive
            struct IndexedPrimitive
            {
                uint32_t posIdx;
                uint32_t normalIdx;
                uint32_t uvIdx;

                bool operator<(const IndexedPrimitive& other) const
                {
                    return memcmp(this, &other, sizeof(IndexedPrimitive)) > 0;
                }
            };
            std::cout << shapes.size() << "\n";
            std::map<IndexedPrimitive, uint32_t> indexedPrims;
            for (size_t s = 0; s < shapes.size(); s++)
            {
                const tinyobj::shape_t& shape = shapes[s];

                uint32_t meshIdxBase = indexBuffer.size();
                for (size_t i = 0; i < shape.mesh.indices.size(); i++)
                {
                    auto index = shape.mesh.indices[i];

                    // Fetch indices to construct an IndexedPrimitive to first look up existing unique vertices
                    int vtxIdx = index.vertex_index;
                    assert(vtxIdx != -1);

                    bool hasNormals = index.normal_index != -1;
                    bool hasUV = index.texcoord_index != -1;

                    int normalIdx = index.normal_index;
                    int uvIdx = index.texcoord_index;

                    IndexedPrimitive prim;
                    prim.posIdx = vtxIdx;
                    prim.normalIdx = hasNormals ? normalIdx : UINT32_MAX;
                    prim.uvIdx = hasUV ? uvIdx : UINT32_MAX;

                    auto res = indexedPrims.find(prim);
                    if (res != indexedPrims.end())
                    {
                        // Vertex is already defined in terms of POS/NORMAL/UV indices, just append index data to index buffer
                        indexBuffer.push_back(res->second);
                    }
                    else
                    {
                        // New unique vertex found, get vertex data and append it to vertex buffer and update indexed primitives
                        auto newIdx = vertexBuffer.size();
                        indexedPrims[prim] = newIdx;
                        indexBuffer.push_back(newIdx);

                        auto vx = attribs.vertices[3 * index.vertex_index];
                        auto vy = attribs.vertices[3 * index.vertex_index + 1];
                        auto vz = attribs.vertices[3 * index.vertex_index + 2];

                        glm::vec3 pos(vx, vy, vz);

                        glm::vec3 normal(0.f);
                        if (hasNormals)
                        {
                            auto nx = attribs.normals[3 * index.normal_index];
                            auto ny = attribs.normals[3 * index.normal_index + 1];
                            auto nz = attribs.normals[3 * index.normal_index + 2];

                            normal.x = nx;
                            normal.y = ny;
                            normal.z = nz;
                        }

                        glm::vec2 uv(0.f);
                        if (hasUV)
                        {
                            auto ux = attribs.texcoords[2 * index.texcoord_index];
                            auto uy = 1.f - attribs.texcoords[2 * index.texcoord_index + 1];

                            uv.s = glm::abs(ux);
                            uv.t = glm::abs(uy);
                        }

                        VertexInput uniqueVertex = { pos, normal, uv };
                        vertexBuffer.push_back(uniqueVertex);
                    }
                }

                // Push new mesh to be rendered in the scene 
                Mesh mesh;
                mesh.idxOffset = meshIdxBase;
                mesh.idxCount = shape.mesh.indices.size();

                assert((shape.mesh.material_ids[0] != -1) && "Mesh missing a material!");
                mesh.diffuseTexName = materials[shape.mesh.material_ids[0]].diffuse_texname; // No per-face material but fixed one

                meshBuffer.push_back(mesh);
            }
        }
        printf("Obj %s loaded\n", fileName);
        std::cout << "Mesh size is: " << meshBuffer.size() << "\n";
    }
    else
    {
        printf("ERROR: %s\n", err.c_str());
        assert(false && "Failed to load .OBJ file, check file paths!");
    }
}

void Scene()
{
    // Allocate and clear the frame buffer before starting to render to it
    std::vector<glm::vec3> frameBuffer(g_scWidth * g_scHeight, glm::vec3(0, 0, 0)); // clear color black = vec3(0, 0, 0)

    // Allocate and clear the depth buffer to FLT_MAX as we utilize z values to resolve visibility now
    std::vector<float> depthBuffer(g_scWidth * g_scHeight, FLT_MAX);

    // We will have single giant index and vertex buffer to draw indexed meshes
    std::vector<VertexInput> vertexBuffer;
    std::vector<uint32_t> indexBuffer;

    // Store data of all scene objects to be drawn
    std::vector<Mesh> primitives;

    // All texture maps loaded. Every mesh will reference their texture map by name at draw time
    std::map<std::string, Texture*> textures;

#if 1
    const auto fileName = "../assets/sponza.obj";
#else
    const auto fileName = "../assets/cube.obj";
#endif

    // Load .OBJ file and process it to construct a scene of multiple meshes
    InitializeSceneObjects(fileName, primitives, vertexBuffer, indexBuffer, textures);

#if 1
    // Build view & projection matrices (right-handed sysem)
    float nearPlane = 0.125f;
    float farPlane = 5000.f;
    glm::vec3 eye(0, -8.5, -5);
    glm::vec3 lookat(20, 2, 1);
    glm::vec3 up(0, 1, 0);
#else
    // Build view & projection matrices (right-handed sysem)
    float nearPlane = 0.1f;
    float farPlane = 100.f;
    glm::vec3 eye(0, 3.75, 6.5);
    glm::vec3 lookat(0, 0, 0);
    glm::vec3 up(0, 1, 0);
#endif

    glm::mat4 view = glm::lookAt(eye, lookat, up);
    view = glm::rotate(view, glm::radians(-30.f), glm::vec3(0, 1, 0));
    glm::mat4 proj = glm::perspective(glm::radians(60.f), static_cast<float>(g_scWidth) / static_cast<float>(g_scHeight), nearPlane, farPlane);

    glm::mat4 MVP = proj * view;

    // Loop over all objects in the scene and draw them one by one
    for (auto i = 0; i < primitives.size(); i++)
    {
#if TRACY_ENABLE
        ZoneScopedN("Meshes");
#endif
        DrawIndexed(frameBuffer, depthBuffer, vertexBuffer, indexBuffer, primitives[i], MVP, textures[primitives[i].diffuseTexName]);
        std::cout << "Scene Mesh " << i << "/" << primitives.size() << " transformed\n";
    }

    std::cout << "All Scene Meshes have been transformed\n";
    
    // Rendering of one frame is finished, output a .PPM file of the contents of our frame buffer to see what we actually just rendered
    //OutputFrameForScene(frameBuffer, "../../render_scene.ppm");
    OutputPngForScene(frameBuffer, "../../render_scene.png");

    // Clean up resources
    for (const auto& elem : textures)
        delete elem.second;
}

// Vertex Shader to apply perspective projections and also pass vertex attributes to Fragment Shader
glm::vec4 VS(const VertexInput& input, const glm::mat4& MVP, FragmentInput& output)
{
    // Simply pass normal and texture coordinates directly to FS
    output.normal = input.normal;
    output.texCoords = input.texCoords;

    // Output a clip-space vec4 that will be used to rasterize parent triangle
    return (MVP * glm::vec4(input.pos, 1.0f));
}

// Fragment Shader that will be run at every visible pixel on triangles to shade fragments
glm::vec3 FS(const FragmentInput& input, Texture* pTexture)
{
#if 1 // Render textured polygons

    // By using fractional part of texture coordinates only, we will REPEAT (or WRAP) the same texture multiple times
    uint32_t idxS = static_cast<uint32_t>((input.texCoords.s - static_cast<int64_t>(input.texCoords.s)) * pTexture->width - 0.5f);
    uint32_t idxT = static_cast<uint32_t>((input.texCoords.t - static_cast<int64_t>(input.texCoords.t)) * pTexture->height - 0.5f);
    uint32_t idx = (idxT * pTexture->width + idxS) * pTexture->numChannels;

    float r = static_cast<float>(pTexture->data[idx++] * (1.f / 255));
    float g = static_cast<float>(pTexture->data[idx++] * (1.f / 255));
    float b = static_cast<float>(pTexture->data[idx++] * (1.f / 255));

    return glm::vec3(r, g, b);
#else // Render interpolated normals
    return (input.Normals) * glm::vec3(0.5) + glm::vec3(0.5); // transform normal values [-1, 1] -> [0, 1] to visualize better
#endif
}

bool EvaluateEdgeFunction(const glm::vec3& E, const glm::vec2& sample)
{
    // Interpolate edge function at given sample
    float result = (E.x * sample.x) + (E.y * sample.y) + E.z;

    // Apply tie-breaking rules on shared vertices in order to avoid double-shading fragments
    if (result > 0.0f) return true;
    else if (result < 0.0f) return false;

    if (E.x > 0.f) return true;
    else if (E.x < 0.0f) return false;

    if ((E.x == 0.0f) && (E.y < 0.0f)) return false;
    else return true;
}

void DrawIndexed(std::vector<glm::vec3>& frameBuffer, std::vector<float>& depthBuffer, std::vector<VertexInput>& vertexBuffer, std::vector<uint32_t>& indexBuffer, Mesh& mesh, glm::mat4& MVP, Texture* pTexture)
{
    assert(pTexture != nullptr);

    const int32_t triCount = mesh.idxCount / 3;

    // Loop over triangles in a given mesh and rasterize them
    #pragma omp parallel for
    for (int32_t idx = 0; idx < triCount; idx++)
    {
#if TRACY_ENABLE
        ZoneScopedN("Tri Calculations");
#endif
        // Fetch vertex input of next triangle to be rasterized
        const VertexInput& vi0 = vertexBuffer[indexBuffer[mesh.idxOffset + (idx * 3)]];
        const VertexInput& vi1 = vertexBuffer[indexBuffer[mesh.idxOffset + (idx * 3 + 1)]];
        const VertexInput& vi2 = vertexBuffer[indexBuffer[mesh.idxOffset + (idx * 3 + 2)]];

        // To collect VS payload
        FragmentInput fi0;
        FragmentInput fi1;
        FragmentInput fi2;

        // Invoke VS for each vertex of the triangle to transform them from object-space to clip-space (-w, w)
        glm::vec4 v0Clip = VS(vi0, MVP, fi0);
        glm::vec4 v1Clip = VS(vi1, MVP, fi1);
        glm::vec4 v2Clip = VS(vi2, MVP, fi2);

        // Apply viewport transformation
        // Notice that we haven't applied homogeneous division and are still utilizing homogeneous coordinates
        glm::vec4 v0Homogen = TO_RASTER(v0Clip);
        glm::vec4 v1Homogen = TO_RASTER(v1Clip);
        glm::vec4 v2Homogen = TO_RASTER(v2Clip);

        // Base vertex matrix
        glm::mat3 M =
        {
            // Notice that glm is itself column-major)
            { v0Homogen.x, v1Homogen.x, v2Homogen.x},
            { v0Homogen.y, v1Homogen.y, v2Homogen.y},
            { v0Homogen.w, v1Homogen.w, v2Homogen.w},
        };

        // Singular vertex matrix (det(M) == 0.0) means that the triangle has zero area,
        // which in turn means that it's a degenerate triangle which should not be rendered anyways,
        // whereas (det(M) > 0) implies a back-facing triangle so we're going to skip such primitives
        float det = glm::determinant(M);
        if (det >= 0.0f)
            continue;

        // Compute the inverse of vertex matrix to use it for setting up edge & constant functions
        M = inverse(M);

        // Set up edge functions based on the vertex matrix
        // We also apply some scaling to edge functions to be more robust.
        // This is fine, as we are working with homogeneous coordinates and do not disturb the sign of these functions.
        glm::vec3 E0 = M[0] / (glm::abs(M[0].x) + glm::abs(M[0].y));
        glm::vec3 E1 = M[1] / (glm::abs(M[1].x) + glm::abs(M[1].y));
        glm::vec3 E2 = M[2] / (glm::abs(M[2].x) + glm::abs(M[2].y));

        // Calculate constant function to interpolate 1/w
        glm::vec3 C = M * glm::vec3(1, 1, 1);

        // Calculate z interpolation vector
        glm::vec3 Z = M * glm::vec3(v0Clip.z, v1Clip.z, v2Clip.z);

        // Calculate normal interpolation vector
        glm::vec3 PNX = M * glm::vec3(fi0.normal.x, fi1.normal.x, fi2.normal.x);
        glm::vec3 PNY = M * glm::vec3(fi0.normal.y, fi1.normal.y, fi2.normal.y);
        glm::vec3 PNZ = M * glm::vec3(fi0.normal.z, fi1.normal.z, fi2.normal.z);

        // Calculate UV interpolation vector
        glm::vec3 PUVS = M * glm::vec3(fi0.texCoords.s, fi1.texCoords.s, fi2.texCoords.s);
        glm::vec3 PUVT = M * glm::vec3(fi0.texCoords.t, fi1.texCoords.t, fi2.texCoords.t);

        // Start rasterizing by looping over pixels to output a per-pixel color
        for (auto y = 0; y < g_scHeight; y++)
        {
            for (auto x = 0; x < g_scWidth; x++)
            {
                // Sample location at the center of each pixel
                glm::vec2 sample = { x + 0.5f, y + 0.5f };

                // Evaluate edge functions at current fragment
                bool inside0 = EvaluateEdgeFunction(E0, sample);
                bool inside1 = EvaluateEdgeFunction(E1, sample);
                bool inside2 = EvaluateEdgeFunction(E2, sample);

                // If sample is "inside" of all three half-spaces bounded by the three edges of the triangle, it's 'on' the triangle
                if (inside0 && inside1 && inside2)
                {
                    // Interpolate 1/w at current fragment
                    float oneOverW = (C.x * sample.x) + (C.y * sample.y) + C.z;

                    // w = 1/(1/w)
                    float w = 1.f / oneOverW;

                    // Interpolate z that will be used for depth test
                    float zOverW = (Z.x * sample.x) + (Z.y * sample.y) + Z.z;
                    float z = zOverW * w;

                    if (z <= depthBuffer[x + y * g_scWidth])
                    {
                        // Depth test passed; update depth buffer value
                        depthBuffer[x + y * g_scWidth] = z;

                        // Interpolate normal
                        float nxOverW = (PNX.x * sample.x) + (PNX.y * sample.y) + PNX.z;
                        float nyOverW = (PNY.x * sample.x) + (PNY.y * sample.y) + PNY.z;
                        float nzOverW = (PNZ.x * sample.x) + (PNZ.y * sample.y) + PNZ.z;

                        // Interpolate texture coordinates
                        float uOverW = (PUVS.x * sample.x) + (PUVS.y * sample.y) + PUVS.z;
                        float vOverW = (PUVT.x * sample.x) + (PUVT.y * sample.y) + PUVT.z;

                        // Final vertex attributes to be passed to FS
                        glm::vec3 normal = glm::vec3(nxOverW, nyOverW, nzOverW) * w; // {nx/w, ny/w, nz/w} * w -> {nx, ny, nz}
                        glm::vec2 texCoords = glm::vec2(uOverW, vOverW) * w; // {u/w, v/w} * w -> {u, v}

                        // Pass interpolated normal & texture coordinates to FS
                        FragmentInput fsInput = { normal, texCoords };

                        // Invoke fragment shader to output a color for each fragment
                        glm::vec3 outputColor = FS(fsInput, pTexture);

                        // Write new color at this fragment
                        frameBuffer[x + y * g_scWidth] = outputColor;
                    }
                }
            }
        }
    }
}
