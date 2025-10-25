#pragma once
#include "Platform/Window.h"

namespace VulkanCore {

	class WindowsWindow : public Window
	{
	public:
		WindowsWindow() = default;

		WindowsWindow(const WindowSpecs& specs);
		~WindowsWindow();

		using EventCallbackFn = std::function<void(Event&)>;

		void OnUpdate() override;

		const std::string& GetWindowName() const override { return m_Data.Title; }
		bool IsWindowResized() const override { return m_Data.FramebufferResize; }
		void ResetResizeFlag() override { m_Data.FramebufferResize = false; }
		void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }

		// NOTE: In Windows, the window size and framebuffer size are typically the same
		uint32_t GetWindowWidth() const override { return m_Data.Width; }
		uint32_t GetWindowHeight() const override { return m_Data.Height; }
		VkExtent2D GetWindowExtent() const override { return { (uint32_t)m_Data.Width, (uint32_t)m_Data.Height }; }

		uint32_t GetFramebufferWidth() const override { return m_Data.Width; }
		uint32_t GetFramebufferHeight() const override { return m_Data.Height; }
		VkExtent2D GetFramebufferExtent() const override { return { (uint32_t)m_Data.Width, (uint32_t)m_Data.Height }; }

		void* GetNativeWindow() override { return m_Window; }
	private:
		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
		void SetWindowTitleDarkMode();

		void Init(const WindowSpecs& specs);
		void Shutdown();
	private:
		GLFWwindow* m_Window;

		struct WindowData
		{
			std::string Title;
			int Width, Height;
			bool FramebufferResize;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};
}
