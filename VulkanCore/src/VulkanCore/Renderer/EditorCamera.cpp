#include "vulkanpch.h"
#include "EditorCamera.h"
#include "VulkanCore/Events/Input.h"

#include "VulkanCore/Core/Core.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace VulkanCore {

	EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
		: Camera(fov, aspectRatio, nearClip, farClip)
	{
		m_ProjectionMatrix = glm::perspective(fov, aspectRatio, nearClip, farClip);
		UpdateView();
	}

	void EditorCamera::UpdateProjection()
	{
		//m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
		m_ProjectionMatrix = glm::perspective(m_FOV, m_AspectRatio, m_NearClip, m_FarClip);
	}

	void EditorCamera::UpdateView()
	{
		// m_Yaw = m_Pitch = 0.0f; // Lock the camera's rotation
		m_Position = CalculatePosition();

		glm::quat orientation = GetOrientation();
		m_InverseViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
		m_ViewMatrix = glm::inverse(m_InverseViewMatrix);
	}

	std::pair<float, float> EditorCamera::PanSpeed() const
	{
		float x = std::min(m_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = std::min(m_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float EditorCamera::RotationSpeed() const
	{
		return 0.8f;
	}

	float EditorCamera::DragSpeed() const
	{
		return 5.0f;
	}

	float EditorCamera::ZoomSpeed() const
	{
		float distance = m_Distance * 0.2f;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;
		speed = std::min(speed, 100.0f); // max speed = 100
		return speed;
	}

	void EditorCamera::OnUpdate()
	{
		if (m_FlyMode)
		{
			auto mousePosition = Input::GetMousePosition();

			const glm::vec2& mouse{ mousePosition.first, mousePosition.second };
			glm::vec2 delta = (mouse - m_InitialMousePosition) * 0.003f;
			m_InitialMousePosition = mouse;

			Input::SetCursorMode(CursorMode::Locked);
			MouseRotate(delta);
		}
		else if (Input::IsKeyPressed(Key::LeftAlt))
		{
			auto mousePosition = Input::GetMousePosition();

			const glm::vec2& mouse{ mousePosition.first, mousePosition.second };
			glm::vec2 delta = (mouse - m_InitialMousePosition) * 0.003f;
			m_InitialMousePosition = mouse;

			if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
				MousePan(delta);
			else if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
				MouseRotate(delta);
			else if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
				MouseZoom(delta.y);
		}

		UpdateView();
	}

	void EditorCamera::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(VK_CORE_BIND_EVENT_FN(EditorCamera::OnMouseScroll));
		dispatcher.Dispatch<KeyPressedEvent>(VK_CORE_BIND_EVENT_FN(EditorCamera::OnKeyEvent));
	}

	void EditorCamera::SetFly(bool flyMode)
	{
		m_FlyMode = flyMode;
		Input::SetCursorMode(flyMode ? CursorMode::Locked : CursorMode::Normal);
	}

	void EditorCamera::SetFocalPoint(const glm::vec3& focalPoint)
	{
		m_FocalPoint = focalPoint;
	}

	bool EditorCamera::OnKeyEvent(KeyPressedEvent& e)
	{
		if (m_FlyMode)
		{
			switch (e.GetKeyCode())
			{
			case Key::W:
			{
				m_FocalPoint += GetForwardDirection() * m_FlySpeed;
				break;
			}
			case Key::A:
			{
				m_FocalPoint -= GetRightDirection() * m_FlySpeed;
				break;
			}
			case Key::S:
			{
				m_FocalPoint -= GetForwardDirection() * m_FlySpeed;
				break;
			}
			case Key::D:
			{
				m_FocalPoint += GetRightDirection() * m_FlySpeed;
				break;
			}
			}
		}

		return true;
	}

	bool EditorCamera::OnMouseScroll(MouseScrolledEvent& e)
	{
		if (m_FlyMode)
		{
			m_FlySpeed += e.GetYOffset() * 0.01f;
			m_FlySpeed = glm::max(m_FlySpeed, 0.01f);
		}
		else
		{
			float delta = e.GetYOffset() * 0.1f;
			MouseZoom(delta);
		}

		UpdateView();
		return true;
	}

	void EditorCamera::MousePan(const glm::vec2& delta)
	{
		auto [xSpeed, ySpeed] = PanSpeed();
		m_FocalPoint += -GetRightDirection() * delta.x * xSpeed * m_Distance;
		m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
	}

	void EditorCamera::MouseRotate(const glm::vec2& delta)
	{
		float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
		m_Yaw += yawSign * delta.x * RotationSpeed();
		m_Pitch += delta.y * RotationSpeed();
	}

	void EditorCamera::MouseDrag(const glm::vec2& delta)
	{
		glm::vec2 abs_delta = glm::abs(delta);
		if (abs_delta.x < abs_delta.y)
			m_FocalPoint += -GetForwardDirection() * delta.y * DragSpeed();

		else
		{
			float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
			m_Yaw += yawSign * delta.x * RotationSpeed();
		}
	}

	void EditorCamera::MouseZoom(float delta)
	{
		m_Distance -= delta * ZoomSpeed();
		if (m_Distance < 1.0f)
		{
			m_FocalPoint += GetForwardDirection();
			m_Distance = 1.0f;
		}
	}

	glm::vec3 EditorCamera::GetUpDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetRightDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetForwardDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 EditorCamera::CalculatePosition() const
	{
		return m_FocalPoint - GetForwardDirection() * m_Distance;
	}

	glm::quat EditorCamera::GetOrientation() const
	{
		return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
	}

	void EditorCamera::KeyWalk()
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

}