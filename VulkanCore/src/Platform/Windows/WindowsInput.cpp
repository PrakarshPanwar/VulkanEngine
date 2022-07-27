#include "vulkanpch.h"
#include "Events/Input.h"
#include "Core/VulkanApplication.h"

#include <GLFW/glfw3.h>

namespace VulkanCore {

	bool Input::IsKeyPressed(KeyCode key)
	{
		GLFWwindow* window = (GLFWwindow*)VulkanApplication::Get()->GetWindow()->GetNativeWindow();
		auto state = glfwGetKey(window, key);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::IsMouseButtonPressed(MouseCode button)
	{
		GLFWwindow* window = (GLFWwindow*)VulkanApplication::Get()->GetWindow()->GetNativeWindow();
		auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
		return state == GLFW_PRESS;
	}

	std::pair<float, float> Input::GetMousePosition()
	{
		double xpos, ypos;

		GLFWwindow* window = (GLFWwindow*)VulkanApplication::Get()->GetWindow()->GetNativeWindow();
		glfwGetCursorPos(window, &xpos, &ypos);
		return { (float)xpos, (float)ypos };
	}

}