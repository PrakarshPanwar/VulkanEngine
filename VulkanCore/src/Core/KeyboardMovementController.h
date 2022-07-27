#pragma once
#include "VulkanGameObject.h"
#include "Platform/Windows/WindowsWindow.h"

namespace VulkanCore {

	struct KeyMappings
	{
		int MoveLeft = GLFW_KEY_A;
		int MoveRight = GLFW_KEY_D;
		int MoveForward = GLFW_KEY_W;
		int MoveBackward = GLFW_KEY_S;
		int MoveUp = GLFW_KEY_E;
		int MoveDown = GLFW_KEY_Q;
		int LookLeft = GLFW_KEY_LEFT;
		int LookRight = GLFW_KEY_RIGHT;
		int LookUp = GLFW_KEY_UP;
		int LookDown = GLFW_KEY_DOWN;
	};

	class KeyboardMovementController
	{
	public:
		void MoveInPlaneXZ(GLFWwindow* window, float timestep, VulkanGameObject& gameObject);
	private:
		KeyMappings m_KeyMappings{};
		float m_MoveSpeed{ 3.0f };
		float m_LookSpeed{ 1.5f };
	};

}