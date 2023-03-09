#pragma once
#include "Scene.hpp"

class Rasterizer
{
public:

	//3840 x 2160 pixels
	//static constexpr std::uint32_t DEFAULT_WIDTH = 3840u;
	//static constexpr std::uint32_t DEFAULT_HEIGHT = 2160u;
	static constexpr std::uint32_t DEFAULT_WIDTH = 7680u;
	static constexpr std::uint32_t DEFAULT_HEIGHT = 4320u;

	/// <summary>
	/// Creates a rasterizer
	/// </summary>
	/// <param name="scene">Scene to be given to the rasterizer</param>
	Rasterizer(Scene&& scene);
	/// <summary>
	/// Creates a Rasterizer
	/// </summary>
	/// <param name="scene">scene to be given to the rasterizer</param>
	/// <param name="height">screen height</param>
	/// <param name="width">screen width</param>
	Rasterizer(Scene&& scene, std::uint32_t width, std::uint32_t height);

	void TransformScene();

	void RenderToPng(std::string_view filename);


	std::uint32_t GetScreenWidth() { return m_ScreenWidth; }
	std::uint32_t GetScreenHeight() { return m_ScreenHeight; }
	std::vector<glm::vec3> GetFrameBuffer() { return m_FrameBuffer; }
	std::vector<float> GetDepthBuffer() { return m_DepthBuffer; }


private:
	
	Scene m_Scene{};

	std::uint32_t m_ScreenWidth{};
	std::uint32_t m_ScreenHeight{};

	std::vector<glm::vec3> m_FrameBuffer{};
	std::vector<float> m_DepthBuffer{};

	glm::vec4 Raster(glm::vec4 vec);

	void InitBuffers();

    glm::vec4 VertexShader(const VertexInput& input, const glm::mat4& MVP, FragmentInput& output);

	glm::vec3 FragmentShader(const FragmentInput& input, Texture* pTexture);

	bool EvaluateEdgeFunction(const glm::vec3& E, const glm::vec2& sample);

};