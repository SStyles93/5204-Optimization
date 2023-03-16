#include "Rasterizer.hpp"

#if TRACY_ENABLE
#include "tracy/Tracy.hpp"
#endif // TRACY_ENABLE


Rasterizer::Rasterizer(Scene&& scene) : Rasterizer(std::move(scene), DEFAULT_WIDTH, DEFAULT_HEIGHT) {}

Rasterizer::Rasterizer(Scene&& scene, std::uint32_t width, std::uint32_t height)
	: m_Scene(std::move(scene)), m_ScreenWidth(width), m_ScreenHeight(height)
{
	InitBuffers();
}

glm::vec4 Rasterizer::Raster(glm::vec4 vec)
{
	return glm::vec4((m_ScreenWidth * (vec.x + vec.w) / 2), (m_ScreenHeight * (vec.w - vec.y) / 2), vec.z, vec.w);
}

void Rasterizer::InitBuffers()
{
	// Allocate and clear the frame buffer before starting to render to it
	m_FrameBuffer = std::vector<glm::vec3>(m_ScreenWidth * m_ScreenHeight, glm::vec3(0, 0, 0)); // clear color black = vec3(0, 0, 0)

	// Allocate and clear the depth buffer to FLT_MAX as we utilize z values to resolve visibility now
	m_DepthBuffer = std::vector<float>(m_ScreenWidth * m_ScreenHeight, FLT_MAX);
}

// Vertex Shader to apply perspective projections and also pass vertex attributes to Fragment Shader
glm::vec4 Rasterizer::VertexShader(const VertexInput& input, const glm::mat4& MVP, FragmentInput& output)
{
	// Simply pass normal and texture coordinates directly to FragmentShader
	output.normal = input.normal;
	output.texCoords = input.texCoords;

	// Output a clip-space vec4 that will be used to rasterize parent triangle
	return (MVP * glm::vec4(input.pos, 1.0f));
}

// Fragment Shader that will be run at every visible pixel on triangles to shade fragments
glm::vec3 Rasterizer::FragmentShader(const FragmentInput& input, Texture* pTexture)
{
	// By using fractional part of texture coordinates only, we will REPEAT (or WRAP) the same texture multiple times
	uint32_t idxS = static_cast<uint32_t>((input.texCoords.s - static_cast<int64_t>(input.texCoords.s)) * pTexture->width - 0.5f);
	uint32_t idxT = static_cast<uint32_t>((input.texCoords.t - static_cast<int64_t>(input.texCoords.t)) * pTexture->height - 0.5f);
	uint32_t idx = (idxT * pTexture->width + idxS) * pTexture->numChannels;

	float r = static_cast<float>(pTexture->data[idx++] * (1.f / 255));
	float g = static_cast<float>(pTexture->data[idx++] * (1.f / 255));
	float b = static_cast<float>(pTexture->data[idx++] * (1.f / 255));

	return glm::vec3(r, g, b);
	////Used to see normals
	//return (input.normal) * glm::vec3(0.5) + glm::vec3(0.5);
}

bool Rasterizer::EvaluateEdgeFunction(const glm::vec3& E, const glm::vec2& sample)
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

void Rasterizer::TransformScene()
{

	for (int i = 0; i < m_Scene.primitives.size(); i++)
	{
#if TRACY_ENABLE
		ZoneScopedN("Meshes");
#endif
		const int32_t triCount = m_Scene.primitives[i].idxCount / 3;

		// Loop over triangles in a given scene.primitives[i] and rasterize them
		//Time : 3:00.1 (best config for omp
#pragma omp parallel for
		for (int32_t idx = 0; idx < triCount; idx++)
		{
#if TRACY_ENABLE
			ZoneScopedN("Tri Calculations");
#endif
			// Fetch vertex input of next triangle to be rasterized
			const VertexInput& vi0 = m_Scene.vertexBuffer[m_Scene.indexBuffer[m_Scene.primitives[i].idxOffset + (idx * 3)]];
			const VertexInput& vi1 = m_Scene.vertexBuffer[m_Scene.indexBuffer[m_Scene.primitives[i].idxOffset + (idx * 3 + 1)]];
			const VertexInput& vi2 = m_Scene.vertexBuffer[m_Scene.indexBuffer[m_Scene.primitives[i].idxOffset + (idx * 3 + 2)]];

			// To collect VertexShader payload
			FragmentInput fi0;
			FragmentInput fi1;
			FragmentInput fi2;

			// Invoke VertexShader for each vertex of the triangle to transform them from object-space to clip-space (-w, w)
			glm::vec4 v0Clip = VertexShader(vi0, m_Scene.GetCamera().MVP, fi0);
			glm::vec4 v1Clip = VertexShader(vi1, m_Scene.GetCamera().MVP, fi1);
			glm::vec4 v2Clip = VertexShader(vi2, m_Scene.GetCamera().MVP, fi2);

			// Apply viewport transformation
			// Notice that we haven't applied homogeneous division and are still utilizing homogeneous coordinates
			glm::vec4 v0Homogen = Raster(v0Clip);
			glm::vec4 v1Homogen = Raster(v1Clip);
			glm::vec4 v2Homogen = Raster(v2Clip);

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
			glm::vec3 PUVertexShader = M * glm::vec3(fi0.texCoords.s, fi1.texCoords.s, fi2.texCoords.s);
			glm::vec3 PUVT = M * glm::vec3(fi0.texCoords.t, fi1.texCoords.t, fi2.texCoords.t);

			// Start rasterizing by looping over pixels to output a per-pixel color
			for (auto y = 0; y < m_ScreenHeight; y++)
			{
#if TRACY_ENABLE
				ZoneScopedN("EdgeEval");
#endif


				for (auto x = 0; x < m_ScreenWidth; x++)
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

						if (z <= m_DepthBuffer[x + y * m_ScreenWidth])
						{
							// Depth test passed; update depth buffer value
							m_DepthBuffer[x + y * m_ScreenWidth] = z;

							// Interpolate normal
							float nxOverW = (PNX.x * sample.x) + (PNX.y * sample.y) + PNX.z;
							float nyOverW = (PNY.x * sample.x) + (PNY.y * sample.y) + PNY.z;
							float nzOverW = (PNZ.x * sample.x) + (PNZ.y * sample.y) + PNZ.z;

							// Interpolate texture coordinates
							float uOverW = (PUVertexShader.x * sample.x) + (PUVertexShader.y * sample.y) + PUVertexShader.z;
							float vOverW = (PUVT.x * sample.x) + (PUVT.y * sample.y) + PUVT.z;

							// Final vertex attributes to be passed to FragmentShader
							glm::vec3 normal = glm::vec3(nxOverW, nyOverW, nzOverW) * w; // {nx/w, ny/w, nz/w} * w -> {nx, ny, nz}
							glm::vec2 texCoords = glm::vec2(uOverW, vOverW) * w; // {u/w, v/w} * w -> {u, v}

							// Pass interpolated normal & texture coordinates to FragmentShader
							FragmentInput FragmentShaderInput = { normal, texCoords };

							// Invoke fragment shader to output a color for each fragment
							Texture* pTexture = m_Scene.textures.at(m_Scene.primitives[i].diffuseTexName);
							glm::vec3 outputColor = FragmentShader(FragmentShaderInput, pTexture);

							// Write new color at this fragment
							m_FrameBuffer[x + y * m_ScreenWidth] = outputColor;
						}
					}
				}


			}
		}
		std::cout << "Scene Mesh " << i << "/" << m_Scene.primitives.size() << " transformed\n";
	}
	std::cout << "All Scene Meshes have been transformed\n";
}

void Rasterizer::RenderToPng(const std::string_view filename)
{
	assert(m_FrameBuffer.size() >= (m_ScreenWidth * m_ScreenHeight));

	FILE* pFile = nullptr;
	fopen_s(&pFile, filename.data(), "wb"); // Use binary mode for writing
	assert(pFile != nullptr);

	// Initialize the PNG writer
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	assert(png_ptr != nullptr);

	png_infop info_ptr = png_create_info_struct(png_ptr);
	assert(info_ptr != nullptr);

	png_init_io(png_ptr, pFile);

	// Write the PNG header
	png_set_IHDR(png_ptr, info_ptr, m_ScreenWidth, m_ScreenHeight, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);

	// Allocate a row buffer for writing the image data
	png_bytep row = (png_bytep)malloc(m_ScreenWidth * 3);

	// Write the image data, row by row
	for (auto y = 0; y < m_ScreenHeight; ++y)
	{
		for (auto x = 0; x < m_ScreenWidth; ++x)
		{
			// Get the pixel color values, clamped to [0, 255]
			uint32_t r = static_cast<uint32_t>(255 * glm::clamp(m_FrameBuffer[y * m_ScreenWidth + x].r, 0.0f, 1.0f));
			uint32_t g = static_cast<uint32_t>(255 * glm::clamp(m_FrameBuffer[y * m_ScreenWidth + x].g, 0.0f, 1.0f));
			uint32_t b = static_cast<uint32_t>(255 * glm::clamp(m_FrameBuffer[y * m_ScreenWidth + x].b, 0.0f, 1.0f));

			// Write the pixel color values to the row buffer
			row[x * 3 + 0] = static_cast<png_byte>(r);
			row[x * 3 + 1] = static_cast<png_byte>(g);
			row[x * 3 + 2] = static_cast<png_byte>(b);
		}

		// Write the row buffer to the PNG file
		png_write_row(png_ptr, row);
		std::cout << "Png line " << y << " had been writen\n";
	}

	// Clean up
	free(row);
	png_write_end(png_ptr, nullptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(pFile);
}
