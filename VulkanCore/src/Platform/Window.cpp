#include "vulkanpch.h"
#include "Window.h"

#include "Windows/WindowsWindow.h"
#include "Linux/LinuxWindow.h"

namespace VulkanCore {

	std::shared_ptr<Window> Window::Create(const WindowSpecs& specs)
	{
#if defined(_WIN32)
		return std::make_shared<WindowsWindow>(specs);
#elif defined(__linux__)
		return std::make_shared<LinuxWindow>(specs);
#endif
	}

}
