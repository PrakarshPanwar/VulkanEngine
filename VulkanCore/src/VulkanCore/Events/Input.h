#pragma once
#include "VulkanCore/Events/KeyCodes.h"
#include "VulkanCore/Events/MouseCodes.h"

namespace VulkanCore {

	enum class CursorMode
	{
		Normal,
		Hidden,
		Locked
	};

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);
		static void SetCursorMode(CursorMode cursorMode);
		static bool IsMouseButtonPressed(MouseCode button);
		static std::pair<float, float> GetMousePosition();
	};

}
