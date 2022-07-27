#pragma once
#include <glm/glm.hpp>
#include <glm/ext/quaternion_common.hpp>

namespace VulkanCore {

	class Camera
	{
	public:
		Camera();
		~Camera();

		void SetOrthographicProjection(float left, float right, float top, float bottom, float near, float far);
		void SetPerspectiveProjection(float fovy, float aspect, float near, float far);
		void SetViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f));
		void SetViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f));
		void SetViewYXZ(glm::vec3 position, glm::vec3 rotation);

		inline const glm::mat4& GetProjectionMatrix() const { return m_Projection; }
		inline const glm::mat4& GetViewMatrix() const { return m_View; }
		inline const glm::mat4& GetInverseViewMatrix() const { return glm::inverse(m_View); }
	private:
		glm::mat4 m_Projection{ 1.0f };
		glm::mat4 m_View{ 1.0f };
		glm::mat4 m_InverseView{ 1.0f };
	};

}