#include "vulkanpch.h"
#include "Window.h"
#include "VulkanCore/Core/Core.h"

#include "Windows/WindowsWindow.h"
#include "Linux/LinuxWindow.h"

namespace VulkanCore {

	std::shared_ptr<Window> Window::Create(const WindowSpecs& specs)
	{
#ifdef VK_PLATFORM_WINDOWS
		return std::make_shared<WindowsWindow>(specs);
#elif defined(VK_PLATFORM_LINUX)
		return std::make_shared<LinuxWindow>(specs);
#endif
	}

}
