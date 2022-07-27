#pragma once
#include "Events/KeyCodes.h"
#include "Events/MouseCodes.h"

namespace VulkanCore {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);
		static bool IsMouseButtonPressed(MouseCode button);
		static std::pair<float, float> GetMousePosition();
	};

}