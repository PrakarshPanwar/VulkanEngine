#pragma once
#include "VulkanCore/Events/KeyCodes.h"
#include "VulkanCore/Events/MouseCodes.h"

namespace VulkanCore {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);
		static bool IsMouseButtonPressed(MouseCode button);
		static std::pair<float, float> GetMousePosition();
	};

}