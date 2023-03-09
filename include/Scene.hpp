#pragma once
#include <iostream>

#include <fstream>

#include <vector>
#include <cassert>
#include <cstdint>
#include <chrono>
#include <vector>
#include <benchmark/benchmark.h>
#include <tiny_obj_loader.h>
#include <png.h>
#include <stb_image.h>

#include "Camera.hpp"

// Used for texture mapping
struct Texture
{
	stbi_uc* data = nullptr;
	std::int32_t width = -1;
	std::int32_t height = -1;
	std::int32_t numChannels = -1;
};

// Vertex data to be fed into each VertexShader invocation as input
struct VertexInput
{
	glm::vec3   pos;
	glm::vec3   normal;
	glm::vec2   texCoords;
};

// Vertex Shader payload, which will be passed to each FragmentShader invocation as input
struct FragmentInput
{
	glm::vec3   normal;
	glm::vec2   texCoords;
};

// Indexed mesh
struct Mesh
{
	// Offset into the global index buffer
	std::uint32_t idxOffset = 0u;

	// How many indices this mesh contains. Number of triangles therefore equals (m_IdxCount / 3)
	std::uint32_t idxCount = 0u;

	// Texture map from material
	std::string diffuseTexName;
};

// POD of indices of vertex data provided by tinyobjloader, used to map unique vertex data to indexed primitive
struct IndexedPrimitive
{
	std::uint32_t posIdx;
	std::uint32_t normalIdx;
	std::uint32_t uvIdx;

	bool operator<(const IndexedPrimitive& other) const
	{
		return memcmp(this, &other, sizeof(IndexedPrimitive)) > 0;
	}
};

class Scene
{
public:

	std::vector<VertexInput> vertexBuffer{};
	std::vector<uint32_t> indexBuffer{};
	std::vector<Mesh> primitives{};
	std::map<std::string, Texture*> textures{};

	/// <summary>
	/// Constructor
	/// </summary>
	Scene();
	Scene(Camera camera);
	Scene(const Scene& other) = delete;
	Scene(Scene&& other) noexcept;
	~Scene();

	Scene& operator=(const Scene& other) = delete;
	Scene& operator=(Scene&& other) noexcept;

	void LoadObject(std::string_view fileName);

	Camera GetCamera() { return m_Camera; }

private:

	Camera m_Camera{};
};
