#include "vulkanpch.h"
#include "WindowsWindow.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Application.h"

#include "VulkanCore/Events/ApplicationEvent.h"
#include "VulkanCore/Events/MouseEvent.h"
#include "VulkanCore/Events/KeyEvent.h"

#ifdef VK_PLATFORM_WINDOWS
#include <Windows.h>
#include <CommCtrl.h>
#include <dwmapi.h>

typedef HRESULT(WINAPI* pFnDwmSetWindowAttribute)(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

namespace VulkanCore {

	WindowsWindow::WindowsWindow(const WindowSpecs& specs)
	{
		Init(specs);
	}

	WindowsWindow::~WindowsWindow()
	{
		Shutdown();
	}

	void WindowsWindow::OnUpdate()
	{
		VK_CORE_PROFILE();
		glfwPollEvents();
	}

	void WindowsWindow::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto windowData = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));

		windowData->Width = width;
		windowData->Height = height;
		windowData->FramebufferResize = true;
	}

	void WindowsWindow::SetWindowTitleDarkMode()
	{
#ifdef VK_PLATFORM_WINDOWS
		// Get Windows Handle
		HWND hwnd = glfwGetWin32Window(m_Window);

		// Load DWM Library
		HMODULE dwmmod = LoadLibrary(L"dwmapi.dll");
		if (dwmmod)
		{
			pFnDwmSetWindowAttribute DwmSetWindowAttribute;
			DwmSetWindowAttribute = (pFnDwmSetWindowAttribute)GetProcAddress(dwmmod, "DwmSetWindowAttribute");

			// Title Color
			BOOL useDarkMode = TRUE;

			HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
			if (hr != S_OK)
				VK_CORE_WARN("Failed to Set Windows Window Title Bar Color!");
		}
#endif
	}

	void WindowsWindow::Init(const WindowSpecs& specs)
	{
		m_Data.Title = specs.Name;
		m_Data.Width = specs.Width;
		m_Data.Height = specs.Height;

		int status = glfwInit();
		VK_CORE_ASSERT(status, "Failed to Initialize GLFW!");

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		if (Application::Get()->GetSpecification().Fullscreen)
		{
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);

			m_Data.Width = mode->width;
			m_Data.Height = mode->height;

			glfwWindowHint(GLFW_RED_BITS, mode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
			glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

			m_Window = glfwCreateWindow(m_Data.Width, m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);
		}
		else
			m_Window = glfwCreateWindow(m_Data.Width, m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);

		VK_CORE_INFO("Creating Windows Window '{0}' ({1}, {2})", m_Data.Title, m_Data.Width, m_Data.Height);
		glfwMakeContextCurrent(m_Window);

		glfwSetWindowUserPointer(m_Window, &m_Data);
		glfwSetFramebufferSizeCallback(m_Window, FramebufferResizeCallback);

		// Get Framebuffer Size
		glfwGetFramebufferSize(m_Window, &m_Data.Width, &m_Data.Height);

		// Set GLFW Callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EventCallback(event);
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			WindowCloseEvent event;
			data.EventCallback(event);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int modes)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				KeyPressedEvent event(key, 0);
				data.EventCallback(event);
				break;
			}
			case GLFW_RELEASE:
			{
				KeyReleasedEvent event(key);
				data.EventCallback(event);
				break;
			}
			case GLFW_REPEAT:
			{
				KeyPressedEvent event(key, 1);
				data.EventCallback(event);
				break;
			}
			}
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int modes)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				MouseButtonPressedEvent event(button);
				data.EventCallback(event);
				break;
			}
			case GLFW_RELEASE:
			{
				MouseButtonReleasedEvent event(button);
				data.EventCallback(event);
				break;
			}
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallback(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseMovedEvent event((float)xPos, (float)yPos);
			data.EventCallback(event);
		});

		SetWindowTitleDarkMode();
	}

	void WindowsWindow::Shutdown()
	{
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

}
