#pragma once

#define ARR_SIZE(x) (sizeof(x) / sizeof(x[0]))

// Silence macro redefinition warnings
#undef TO_RASTER
// Transform a given vertex in clip-space [-w,w] to raster-space [0, {w|h}]
#define TO_RASTER(v) glm::vec4((g_scWidth * (v.x + v.w) / 2), (g_scHeight * (v.w - v.y) / 2), v.z, v.w)

    void OutputFrameForCube(const std::vector<glm::vec3>& frameBuffer, const char* filename)
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

    void OutputPngForCube(const std::vector<glm::vec3>& frameBuffer, const char* filename)
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

    void InitializeSceneObjects(std::vector<glm::mat4>& objects)
    {
        // Construct a scene of few cubes randomly positioned

        const glm::mat4 identity(1.f);

        glm::mat4 Matrix0 = glm::translate(identity, glm::vec3(0, 0, -8.f));
        Matrix0 = glm::rotate(Matrix0, glm::radians(0.f), glm::vec3(0, 1, 0));
        
        glm::mat4 Matrix1 = glm::translate(identity, glm::vec3(-3.f, 0, -4.f));
        Matrix1 = glm::rotate(Matrix1, glm::radians(45.f), glm::vec3(0, 1, 0));

        glm::mat4 Matrix2 = glm::translate(identity, glm::vec3(3.f, 0, -4.f));
        Matrix2 = glm::rotate(Matrix2, glm::radians(-45.f), glm::vec3(0, 1, 0));

        glm::mat4 Matrix3 = glm::translate(identity, glm::vec3(0, -2, 0.f));
        Matrix3 = glm::rotate(Matrix3, glm::radians(45.f), glm::vec3(0, 0, 1));

    /*  glm::mat4 M4 = glm::translate(identity, glm::vec3(6.f, 6.f, 0.f));
        M4 = glm::rotate(M3, glm::radians(45.f), glm::vec3(0, 0, 1));*/

        // Change the order of cubes being rendered, see how it changes with and without depth test
        objects.push_back(Matrix0);
        objects.push_back(Matrix2);
        objects.push_back(Matrix1);
        objects.push_back(Matrix3);
        //objects.push_back(M4);
    }

    glm::vec4 VS(const glm::vec3& pos, const glm::mat4& M, const glm::mat4& V, const glm::mat4& P)
    {
        return (P * V * M * glm::vec4(pos, 1.0f));
    }

    bool EvaluateEdgeFunction(const glm::vec3& E, const glm::vec3& sample)
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

    void Cube()
    {
        // Setup vertices & indices to draw an indexed cube
        glm::vec3 vertices[] =
        {
            { 1.0f, -1.0f, -1.0f },
            { 1.0f, -1.0f, 1.0f },
            { -1.0f, -1.0f, 1.0f },
            { -1.0f, -1.0f, -1.0f },
            { 1.0f, 1.0f, -1.0f },
            {  1.0f, 1.0f, 1.0f },
            { -1.0f, 1.0f, 1.0f },
            { -1.0f, 1.0f, -1.0f },
        };

        uint32_t indices[] =
        {
            // 6 faces of cube * 2 triangles per-face * 3 vertices per-triangle = 36 indices
            1,3,0, 7,5,4, 4,1,0, 5,2,1, 2,7,3, 0,7,4, 1,2,3, 7,6,5, 4,5,1, 5,6,2, 2,6,7, 0,3,7
        };

        // Use per-face colors
        glm::vec3 colors[] =
        {
            glm::vec3(0, 0, 1),
            glm::vec3(0, 1, 0),
            glm::vec3(0, 1, 1),
            glm::vec3(1, 1, 1),
            glm::vec3(1, 0, 1),
            glm::vec3(1, 1, 0)
        };

        // Allocate and clear the frame buffer before starting to render to it
        std::vector<glm::vec3> frameBuffer(g_scWidth * g_scHeight, glm::vec3(0, 0, 0)); // clear color black = vec3(0, 0, 0)

        // Allocate and clear the depth buffer (not z-buffer but 1/w-buffer in this part!) to 0.0
        std::vector<float> depthBuffer(g_scWidth * g_scHeight, 0.0);

        // Let's draw multiple objects
        std::vector<glm::mat4> objects;
        InitializeSceneObjects(objects);

        // Build view & projection matrices (right-handed sysem)
        float nearPlane = 0.1f;
        float farPlane = 100.f;
        glm::vec3 eye(0, 3.75, 6.5);
        glm::vec3 lookat(0, 0, 0);
        glm::vec3 up(0, 1, 0);

        //Direction on the camera
        glm::mat4 view = glm::lookAt(eye, lookat, up);
        //Projection in perspective 
        glm::mat4 proj = glm::perspective(glm::radians(60.f), static_cast<float>(g_scWidth) / static_cast<float>(g_scHeight), nearPlane, farPlane);

        // Loop over objects in the scene
        for (size_t n = 0; n < objects.size(); n++)
        {
            // Loop over triangles in a given object and rasterize them one by one
            for (uint32_t idx = 0; idx < ARR_SIZE(indices) / 3; idx++)
            {
                // We're gonna fetch object-space vertices from the vertex buffer indexed by the values in index buffer
                // and pass them directly to each VS invocation
                const glm::vec3& v0 = vertices[indices[idx * 3]];
                const glm::vec3& v1 = vertices[indices[idx * 3 + 1]];
                const glm::vec3& v2 = vertices[indices[idx * 3 + 2]];

                // Invoke function for each vertex of the triangle to transform them from object-space to clip-space (-w, w)
                glm::vec4 v0Clip = VS(v0, objects[n], view, proj);
                glm::vec4 v1Clip = VS(v1, objects[n], view, proj);
                glm::vec4 v2Clip = VS(v2, objects[n], view, proj);

                // Apply viewport transformation
                // Notice that we haven't applied homogeneous division and are still utilizing homogeneous coordinates
                glm::vec4 v0Homogen = TO_RASTER(v0Clip);
                glm::vec4 v1Homogen = TO_RASTER(v1Clip);
                glm::vec4 v2Homogen = TO_RASTER(v2Clip);

                // Base vertex matrix
                glm::mat3 Matrix =
                {
                    // Notice that glm is itself column-major)
                    { v0Homogen.x, v1Homogen.x, v2Homogen.x},
                    { v0Homogen.y, v1Homogen.y, v2Homogen.y},
                    { v0Homogen.w, v1Homogen.w, v2Homogen.w},
                };

                // Singular vertex matrix (det(M) == 0.0) means that the triangle has zero area,
                // which in turn means that it's a degenerate triangle which should not be rendered anyways,
                // whereas (det(M) > 0) implies a back-facing triangle so we're going to skip such primitives
                float det = glm::determinant(Matrix);
                if (det >= 0.0f)
                    continue;

                // Compute the inverse of vertex matrix to use it for setting up edge & constant functions
                Matrix = inverse(Matrix);

                // Set up edge functions based on the vertex matrix
                glm::vec3 E0 = Matrix[0];
                glm::vec3 E1 = Matrix[1];
                glm::vec3 E2 = Matrix[2];

                // Calculate constant function to interpolate 1/w
                glm::vec3 C = Matrix * glm::vec3(1, 1, 1);

                // Start rasterizing by looping over pixels to output a per-pixel color
                for (auto y = 0; y < g_scHeight; y++)
                {
                    for (auto x = 0; x < g_scWidth; x++)
                    {
                        // Sample location at the center of each pixel
                        glm::vec3 sample = { x + 0.5f, y + 0.5f, 1.0f };

                        // Evaluate edge functions at current fragment
                        bool inside0 = EvaluateEdgeFunction(E0, sample);
                        bool inside1 = EvaluateEdgeFunction(E1, sample);
                        bool inside2 = EvaluateEdgeFunction(E2, sample);

                        // If sample is "inside" of all three half-spaces bounded by the three edges of the triangle, it's 'on' the triangle
                        if (inside0 && inside1 && inside2)
                        {
                            // Interpolate 1/w at current fragment
                            float oneOverW = (C.x * sample.x) + (C.y * sample.y) + C.z;

                            // Perform depth test with interpolated 1/w value
                            // Notice that the greater 1/w value is, the closer the object would be to our virtual camera,
                            // hence "less_than_equal" comparison is actually oneOverW >= depthBuffer[sample] and not oneOverW <= depthBuffer[sample] here
                            if (oneOverW >= depthBuffer[x + y * g_scWidth])
                            {
                                // Depth test passed; update depth buffer value
                                depthBuffer[x + y * g_scWidth] = oneOverW;

                                // Write new color at this fragment
                                frameBuffer[x + y * g_scWidth] = colors[indices[3 * idx] % 6];
                            }
                        }
                    }
                }
            }
        }

        // Rendering of one frame is finished, output a .PPM file of the contents of our frame buffer to see what we actually just rendered
        //OutputFrameForCube(frameBuffer, "../render_3D_cube.ppm");
        OutputPngForCube(frameBuffer, "../../render_3D_cube.png");
    }
