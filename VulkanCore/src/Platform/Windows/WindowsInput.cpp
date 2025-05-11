#include "vulkanpch.h"
#include "VulkanCore/Events/Input.h"
#include "VulkanCore/Core/Application.h"

#include <GLFW/glfw3.h>

namespace VulkanCore {

	namespace Utils {

		static int GetGLFWCursorMode(CursorMode cursorMode)
		{
			switch (cursorMode)
			{
			case CursorMode::Normal: return GLFW_CURSOR_NORMAL;
			case CursorMode::Hidden: return GLFW_CURSOR_HIDDEN;
			case CursorMode::Locked: return GLFW_CURSOR_DISABLED;
			default:
				VK_CORE_ASSERT(false, "Cursor Mode not found");
				return GLFW_CURSOR_NORMAL;
			}
		}

	}

	bool Input::IsKeyPressed(KeyCode key)
	{
		GLFWwindow* window = (GLFWwindow*)Application::Get()->GetWindow()->GetNativeWindow();
		auto state = glfwGetKey(window, key);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	void Input::SetCursorMode(CursorMode cursorMode)
	{
		GLFWwindow* window = (GLFWwindow*)Application::Get()->GetWindow()->GetNativeWindow();
		glfwSetInputMode(window, GLFW_CURSOR, Utils::GetGLFWCursorMode(cursorMode));
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}

	bool Input::IsMouseButtonPressed(MouseCode button)
	{
		GLFWwindow* window = (GLFWwindow*)Application::Get()->GetWindow()->GetNativeWindow();
		auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
		return state == GLFW_PRESS;
	}

	std::pair<float, float> Input::GetMousePosition()
	{
		double xpos, ypos;

		GLFWwindow* window = (GLFWwindow*)Application::Get()->GetWindow()->GetNativeWindow();
		glfwGetCursorPos(window, &xpos, &ypos);
		return { (float)xpos, (float)ypos };
	}

}
