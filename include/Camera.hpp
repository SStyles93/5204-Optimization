#pragma once
#include <iostream>

#define GLM_FORCE_INLINE
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

static constexpr std::uint32_t SCREEN_WIDTH = 1280u;
static constexpr std::uint32_t SCREEN_HEIGHT = 720u;
static constexpr glm::vec3 UP(0, 1, 0);
static constexpr glm::mat4 IDENTITY(1.f);


class Camera
{
public:

	Camera();

	void SetNearPlane(float value);
	void SetFarPlane(float value);
	/// <summary>
	/// Sets the position of the eye ("camera")
	/// </summary>
	/// <param name="">position</param>
	void SetEyePosition(glm::vec3);
	/// <summary>
	/// Sets the look direction (lookAt)
	/// </summary>
	/// <param name=""></param>
	void SetLookDirection(glm::vec3);
	/// <summary>
	/// Sets the view direction
	/// </summary>
	void SetView(glm::vec3, glm::vec3);
	/// <summary>
	/// Sets the projection
	/// </summary>
	void SetProjection(float);

	void SetViewAngle(const float angle);

	void SetMVP();

	void SetupCamera();

	
	glm::mat4 MVP{};

private:

	float m_nearPlane = 0.01f;
	float m_farPlane = 5000.f;
	//FieldOfView
	float m_projectionAngle = 30.0f;
	float m_viewAngle = -30.0f;

	glm::vec3 m_eye = glm::vec3(0, -8.5, -5);
	glm::vec3 m_lookat = glm::vec3(20, 2, 1);

	glm::mat4 m_view{};
	glm::mat4 m_projection{};

};
