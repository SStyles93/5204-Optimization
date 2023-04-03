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

// Frame buffer dimensions
static constexpr auto g_scWidth = 1280u;
static constexpr auto g_scHeight = 720u;

// Transform a given vertex in NDC [-1,1] to raster-space [0, {w|h}]
//TODO : TO_RASTER -> Function
//From C to C++

#define TO_RASTER(v) glm::vec3((g_scWidth * (v.x + 1.0f) / 2), (g_scHeight * (v.y + 1.f) / 2), 1.0f)

void OutputFrameForTriangle(const std::vector<glm::vec3>& frameBuffer, const char* filename)
{
	assert(frameBuffer.size() >= (g_scWidth * g_scHeight));

	FILE* pFile = nullptr;
	fopen_s(&pFile, filename, "w");
	fprintf(pFile, "P3\n%d %d\n%d\n ", g_scWidth, g_scHeight, 255);
	for (auto i = 0; i < g_scWidth * g_scHeight; ++i)
	{
		// Write out color values clamped to [0, 255] 
		std::uint32_t r = static_cast<std::uint32_t>(255 * glm::clamp(frameBuffer[i].r, 0.0f, 1.0f));
		std::uint32_t g = static_cast<std::uint32_t>(255 * glm::clamp(frameBuffer[i].g, 0.0f, 1.0f));
		std::uint32_t b = static_cast<std::uint32_t>(255 * glm::clamp(frameBuffer[i].b, 0.0f, 1.0f));
		fprintf(pFile, "%d %d %d ", r, g, b);
	}
	fclose(pFile);


}

void OutputPngForTriangle(const std::vector<glm::vec3>& frameBuffer, const char* filename)
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

void Triangle()
{
#if 1 // 2DH (x, y, w) coordinates of our triangle's vertices, in counter-clockwise order
	glm::vec3 v0(-0.5, 0.5, 1.0);
	glm::vec3 v1(0.5, 0.5, 1.0);
	glm::vec3 v2(0.0, -0.5, 1.0);
#else // Or optionally go with this set of vertices to see how this triangle would be rasterized
	glm::vec3 v0(-0.5, -0.5, 1.0);
	glm::vec3 v1(-0.5, 0.5, 1.0);
	glm::vec3 v2(0.5, 0.5, 1.0);
#endif

	// Apply viewport transformation
	v0 = TO_RASTER(v0);
	v1 = TO_RASTER(v1);
	v2 = TO_RASTER(v2);

	// Per-vertex color values
	// Notice how each of these RGB colors corresponds to each vertex defined above
	glm::vec3 c0(1, 0, 0);
	glm::vec3 c1(0, 1, 0);
	glm::vec3 c2(0, 0, 1);

	// Base vertex matrix
	glm::mat3 M = // transpose(glm::mat3(v0, v1, v2));
	{
		// Notice that glm is itself column-major)
		{ v0.x, v1.x, v2.x},
		{ v0.y, v1.y, v2.y},
		{ v0.z, v1.z, v2.z},
	};

	// Compute the inverse of vertex matrix to use it for setting up edge functions
	M = inverse(M);

	// Calculate edge functions based on the vertex matrix once
	glm::vec3 E0 = M * glm::vec3(1, 0, 0); // == M[0]
	glm::vec3 E1 = M * glm::vec3(0, 1, 0); // == M[1]
	glm::vec3 E2 = M * glm::vec3(0, 0, 1); // == M[2]

	// Allocate and clear the frame buffer before starting to render to it
	std::vector<glm::vec3> frameBuffer(g_scWidth * g_scHeight, glm::vec3(0, 0, 0)); // clear color black = vec3(0, 0, 0)

	// Start rasterizing by looping over pixels to output a per-pixel color
	for (auto y = 0; y < g_scHeight; y++)
	{
		for (auto x = 0; x < g_scWidth; x++)
		{
			// Sample location at the center of each pixel
			glm::vec3 sample = { x + 0.5f, y + 0.5f, 1.0f };

			// Evaluate edge functions at every fragment
			float alpha = glm::dot(E0, sample);
			float beta = glm::dot(E1, sample);
			float gamma = glm::dot(E2, sample);

			// If sample is "inside" of all three half-spaces bounded by the three edges of our triangle, it's 'on' the triangle
			if ((alpha >= 0.0f) && (beta >= 0.0f) && (gamma >= 0.0f))
			{
				assert(((alpha + beta + gamma) - 1.0f) <= glm::epsilon<float>());

#if 1 // Blend per-vertex colors defined above using the coefficients from edge functions
				frameBuffer[x + y * g_scWidth] = glm::vec3(c0 * alpha + c1 * beta + c2 * gamma);
#else // Or go with flat color if that's your thing
				frameBuffer[x + y * g_scWidth] = glm::vec3(1, 0, 0);
#endif
			}
		}
	}

	// Rendering of one frame is finished, output a .PPM file of the contents of our frame buffer to see what we actually just rendered
	//OutputFrameForTriangle(frameBuffer, "../render_triangle_2D.ppm");
	OutputPngForTriangle(frameBuffer, "../../render_triangle_2D.png");
}
