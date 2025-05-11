#include "vulkanpch.h"
#include "Camera.h"

#include <glm/ext/matrix_clip_space.hpp>

namespace VulkanCore {

	Camera::Camera(float fov, float aspectRatio, float nearClip, float farClip)
		: m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip)
	{
	}

	Camera::~Camera()
	{
	}

#if 0
	void Camera::SetOrthographicProjection(float left, float right, float top, float bottom, float near, float far)
	{
		m_Projection[0][0] = 2.f / (right - left);
		m_Projection[1][1] = 2.f / (bottom - top);
		m_Projection[2][2] = 1.f / (far - near);
		m_Projection[3][0] = -(right + left) / (right - left);
		m_Projection[3][1] = -(bottom + top) / (bottom - top);
		m_Projection[3][2] = -near / (far - near);
	}

	void Camera::SetPerspectiveProjection(float fovy, float aspect, float near, float far)
	{
		const float tanHalfFovy = tan(fovy / 2.0f);

		m_Projection[0][0] = 1.f / (aspect * tanHalfFovy);
		m_Projection[1][1] = 1.f / (tanHalfFovy);
		m_Projection[2][2] = far / (far - near);
		m_Projection[2][3] = 1.f;
		m_Projection[3][2] = -(far * near) / (far - near);
	}

	void Camera::SetViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up)
	{
		const glm::vec3 w{ glm::normalize(direction) };
		const glm::vec3 u{ glm::normalize(glm::cross(w, up)) };
		const glm::vec3 v{ glm::cross(w, u) };

		m_View = glm::mat4{ 1.f };
		m_View[0][0] = u.x;
		m_View[1][0] = u.y;
		m_View[2][0] = u.z;
		m_View[0][1] = v.x;
		m_View[1][1] = v.y;
		m_View[2][1] = v.z;
		m_View[0][2] = w.x;
		m_View[1][2] = w.y;
		m_View[2][2] = w.z;
		m_View[3][0] = -glm::dot(u, position);
		m_View[3][1] = -glm::dot(v, position);
		m_View[3][2] = -glm::dot(w, position);

		m_InverseView = glm::mat4{ 1.f };
		m_InverseView[0][0] = u.x;
		m_InverseView[0][1] = u.y;
		m_InverseView[0][2] = u.z;
		m_InverseView[1][0] = v.x;
		m_InverseView[1][1] = v.y;
		m_InverseView[1][2] = v.z;
		m_InverseView[2][0] = w.x;
		m_InverseView[2][1] = w.y;
		m_InverseView[2][2] = w.z;
		m_InverseView[3][0] = position.x;
		m_InverseView[3][1] = position.y;
		m_InverseView[3][2] = position.z;
	}

	void Camera::SetViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up /*= glm::vec3(0.0f, -1.0f, 0.0f)*/)
	{
		SetViewDirection(position, target - position, up);
	}

	void Camera::SetViewYXZ(glm::vec3 position, glm::vec3 rotation)
	{
		const float c3 = glm::cos(rotation.z);
		const float s3 = glm::sin(rotation.z);
		const float c2 = glm::cos(rotation.x);
		const float s2 = glm::sin(rotation.x);
		const float c1 = glm::cos(rotation.y);
		const float s1 = glm::sin(rotation.y);
		const glm::vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
		const glm::vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
		const glm::vec3 w{ (c2 * s1), (-s2), (c1 * c2) };

		m_View = glm::mat4{ 1.f };
		m_View[0][0] = u.x;
		m_View[1][0] = u.y;
		m_View[2][0] = u.z;
		m_View[0][1] = v.x;
		m_View[1][1] = v.y;
		m_View[2][1] = v.z;
		m_View[0][2] = w.x;
		m_View[1][2] = w.y;
		m_View[2][2] = w.z;
		m_View[3][0] = -glm::dot(u, position);
		m_View[3][1] = -glm::dot(v, position);
		m_View[3][2] = -glm::dot(w, position);
	}
#endif

}
