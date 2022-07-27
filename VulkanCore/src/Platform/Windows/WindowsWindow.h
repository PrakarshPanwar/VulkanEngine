#pragma once
#include "Core/glfw_vulkan.h"
#include "Platform/Window.h"

namespace VulkanCore {

	struct WindowSpecs
	{
		int Width, Height;
		std::string Name;
		bool FramebufferResize = false;

		WindowSpecs() = default;
		WindowSpecs(int width, int height, const std::string& name)
			: Width(width), Height(height), Name(name) {}
	};

	class WindowsWindow
	{
	public:
		WindowsWindow() = default;

		WindowsWindow(const WindowSpecs& specs);
		~WindowsWindow();

		using EventCallbackFn = std::function<void(Event&)>;

		void OnUpdate();

		inline std::string& GetWindowName() { return m_WindowSpecs.Name; }
		inline bool IsWindowResize() { return m_WindowSpecs.FramebufferResize; }
		void ResetWindowResizeFlag() { m_WindowSpecs.FramebufferResize = false; };
		void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; };

		uint32_t GetWidth() const { return m_WindowSpecs.Width; }
		uint32_t GetHeight() const { return m_WindowSpecs.Height; }
		inline VkExtent2D GetExtent() { return { (uint32_t)m_WindowSpecs.Width, (uint32_t)m_WindowSpecs.Height }; }
		inline void* GetNativeWindow() { return m_Window; }
	private:
		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

		void Init(const WindowSpecs& specs);
		void Shutdown();
	private:
		GLFWwindow* m_Window;

		struct WindowData
		{
			std::string Title;
			uint32_t Width;
			uint32_t Height;
			bool FramebufferResize;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
		WindowSpecs m_WindowSpecs;
	};
}