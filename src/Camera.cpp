#include "Camera.hpp"

Camera::Camera() 
{
	m_view = glm::lookAt(m_eye, m_lookat, UP);
	m_view = glm::rotate(m_view, glm::radians(m_viewAngle), glm::vec3(0, 1, 0));

	m_projection = glm::perspective(
		glm::radians(m_projectionAngle),
		static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT),
		m_nearPlane,
		m_farPlane);

	MVP = m_projection * m_view;
}

void Camera::SetEyePosition(glm::vec3 position)
{
	m_eye = position;
}
void Camera::SetLookDirection(glm::vec3 direction)
{
	m_lookat = direction;
}
void Camera::SetView(glm::vec3 position, glm::vec3 direction)
{
	m_view = glm::lookAt(position, direction, UP);
}
void Camera::SetProjection(float angle)
{
	m_projection = glm::perspective(
		glm::radians(angle),
		static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT),
		m_nearPlane,
		m_farPlane);
}
