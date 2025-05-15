#pragma once
#include <glm/glm.hpp>

namespace VulkanCore {

	class Camera
	{
	public:
		Camera() = default;
		Camera(float fov, float aspectRatio, float nearClip, float farClip);
		~Camera();

		virtual const glm::mat4& GetProjectionMatrix() const = 0;
		virtual const glm::mat4& GetViewMatrix() const = 0;
		virtual const glm::mat4& GetInverseViewMatrix() const = 0;
		virtual glm::mat4 GetViewProjection() const = 0;
	protected:
		float m_FOV = 0.785398f, m_AspectRatio = 1.778f, m_NearClip = 0.1f, m_FarClip = 1000.0f;

		glm::mat4 m_ProjectionMatrix{};
		glm::mat4 m_ViewMatrix{};
		glm::mat4 m_InverseViewMatrix{};
	};

}
