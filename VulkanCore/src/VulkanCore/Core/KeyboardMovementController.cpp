#include "vulkanpch.h"
#include "KeyboardMovementController.h"

namespace VulkanCore {

	void KeyboardMovementController::MoveInPlaneXZ(GLFWwindow* window, float timestep, VulkanGameObject& gameObject)
	{
		glm::vec3 rotate{};

		if (glfwGetKey(window, m_KeyMappings.LookRight) == GLFW_PRESS)
			rotate.y += 1.0f;
		
		if (glfwGetKey(window, m_KeyMappings.LookLeft) == GLFW_PRESS)
			rotate.y -= 1.0f;

		if (glfwGetKey(window, m_KeyMappings.LookUp) == GLFW_PRESS)
			rotate.x += 1.0f;

		if (glfwGetKey(window, m_KeyMappings.LookDown) == GLFW_PRESS)
			rotate.x -= 1.0f;

		if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon())
			gameObject.Transform.Rotation += m_LookSpeed * timestep * glm::normalize(rotate);

		gameObject.Transform.Rotation.x = glm::clamp(gameObject.Transform.Rotation.x, -1.5f, 1.5f);
		gameObject.Transform.Rotation.y = glm::mod(gameObject.Transform.Rotation.y, glm::two_pi<float>());

		float yaw = gameObject.Transform.Rotation.y;
		const glm::vec3 forwardDir{ sin(yaw), 0.0f, cos(yaw) };
		const glm::vec3 rightDir{ forwardDir.z, 0.0f, -forwardDir.x };
		const glm::vec3 upDir{ 0.0f, -1.0f, 0.0f };

		glm::vec3 moveDir{};

		if (glfwGetKey(window, m_KeyMappings.MoveForward) == GLFW_PRESS)
			moveDir += forwardDir;

		if (glfwGetKey(window, m_KeyMappings.MoveBackward) == GLFW_PRESS)
			moveDir -= forwardDir;

		if (glfwGetKey(window, m_KeyMappings.MoveRight) == GLFW_PRESS)
			moveDir += rightDir;

		if (glfwGetKey(window, m_KeyMappings.MoveLeft) == GLFW_PRESS)
			moveDir -= rightDir;

		if (glfwGetKey(window, m_KeyMappings.MoveUp) == GLFW_PRESS)
			moveDir += upDir;

		if (glfwGetKey(window, m_KeyMappings.MoveDown) == GLFW_PRESS)
			moveDir -= upDir;

		if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon())
			gameObject.Transform.Translation += m_MoveSpeed * timestep * glm::normalize(moveDir);
	}

}