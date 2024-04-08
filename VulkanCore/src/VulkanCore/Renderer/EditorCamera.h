#pragma once
#include <glm/glm.hpp>
#include "VulkanCore/Events/KeyEvent.h"
#include "VulkanCore/Events/MouseEvent.h"

namespace VulkanCore {

	class EditorCamera
	{
	public:
		EditorCamera() = default;
		EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);

		void OnUpdate();
		void OnEvent(Event& e);

		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; }
		inline bool GetFlyMode() const { return m_FlyMode; }
		void SetFlyMode(bool flyMode);

		inline void SetViewportSize(float width, float height)
		{ 
			m_ViewportWidth = width;
			m_ViewportHeight = height;
			m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
			UpdateProjection();
		}

		inline float GetAspectRatio() const { return m_AspectRatio; }
		inline void SetAspectRatio(float aspectRatio) { m_AspectRatio = aspectRatio; UpdateProjection(); }
		void SetFocalPoint(const glm::vec3& focalPoint);

		inline float GetFieldOfView() const { return m_FOV; }
		inline void SetFieldOfView(float fov)
		{
			m_FOV = fov;
			UpdateProjection();
		}

		glm::vec2 GetNearFarClip() const { return { m_NearClip, m_FarClip }; }
		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		glm::mat4 GetViewProjection() const { return m_ProjectionMatrix * m_ViewMatrix; }

		glm::vec3 GetUpDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetForwardDirection() const;
		const glm::vec3& GetPosition() const { return m_Position; }
		glm::quat GetOrientation() const;

		float GetPitch() const { return m_Pitch; }
		float GetYaw() const { return m_Yaw; }
	private:
		void UpdateProjection();
		void UpdateView();

		bool OnKeyEvent(KeyPressedEvent& e);
		bool OnMouseScroll(MouseScrolledEvent& e);
		//bool OnWindowResize(WindowResizeEvent& e);

		void KeyWalk();
		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseDrag(const glm::vec2& delta);
		void MouseZoom(float delta);

		glm::vec3 CalculatePosition() const;

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float DragSpeed() const;
		float ZoomSpeed() const;
	private:
		float m_FOV = 45.0f, m_AspectRatio = 1.778f, m_NearClip = 0.1f, m_FarClip = 1000.0f;

		glm::mat4 m_ProjectionMatrix, m_ViewMatrix;
		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };

		glm::vec2 m_InitialMousePosition = { 0.0f, 0.0f };

		float m_Distance = 10.0f, m_FlySpeed = 0.1f;
		float m_Pitch = 0.0f, m_Yaw = 0.0f;

		float m_ViewportWidth = 1280.0f, m_ViewportHeight = 720.0f;
		bool m_FlyMode = false;
	};

}