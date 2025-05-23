#include "vulkanpch.h"
#include "WindowsWindow.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Application.h"

#include "VulkanCore/Events/ApplicationEvent.h"
#include "VulkanCore/Events/MouseEvent.h"
#include "VulkanCore/Events/KeyEvent.h"

namespace VulkanCore {

	Window* Window::Create(const WindowSpecs& specs)
	{
		return new WindowsWindow(specs);
	}

	WindowsWindow::WindowsWindow(const WindowSpecs& specs)
		: m_WindowSpecs(specs)
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
		auto vkWindow = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));

		vkWindow->FramebufferResize = true;
		vkWindow->Width = width;
		vkWindow->Height = height;
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
			m_WindowSpecs.Width = mode->width;
			m_WindowSpecs.Height = mode->height;

			glfwWindowHint(GLFW_RED_BITS, mode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
			glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

			m_Window = glfwCreateWindow(m_WindowSpecs.Width, m_WindowSpecs.Height, m_WindowSpecs.Name.c_str(), nullptr, nullptr);
		}
		else
			m_Window = glfwCreateWindow(m_WindowSpecs.Width, m_WindowSpecs.Height, m_WindowSpecs.Name.c_str(), nullptr, nullptr);

		VK_CORE_INFO("Creating Window '{0}' ({1}, {2})", m_WindowSpecs.Name, m_WindowSpecs.Width, m_WindowSpecs.Height);
		glfwMakeContextCurrent(m_Window);

		glfwSetWindowUserPointer(m_Window, &m_Data);
		glfwSetFramebufferSizeCallback(m_Window, FramebufferResizeCallback);

		//Set GLFW Callbacks
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

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
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
	}

	void WindowsWindow::Shutdown()
	{
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

}
