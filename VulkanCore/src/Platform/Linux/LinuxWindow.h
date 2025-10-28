#pragma once
#include "Platform/Window.h"

namespace VulkanCore {

	class LinuxWindow : public Window
	{
	public:
		LinuxWindow() = default;
		LinuxWindow(const WindowSpecs& specs);
		~LinuxWindow();

		using EventCallbackFn = std::function<void(Event&)>;

		void OnUpdate() override;

		const std::string& GetWindowName() const override { return m_Data.Title; }
		bool IsWindowResized() const override { return m_Data.FramebufferResize; }
		void ResetResizeFlag() override { m_Data.FramebufferResize = false; };
		void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }

		uint32_t GetWindowWidth() const override { return m_Data.WindowWidth; }
		uint32_t GetWindowHeight() const override { return m_Data.WindowHeight; }
		VkExtent2D GetWindowExtent() const override { return { (uint32_t)m_Data.WindowWidth, (uint32_t)m_Data.WindowHeight }; }

		uint32_t GetFramebufferWidth() const override { return m_Data.FramebufferWidth; }
		uint32_t GetFramebufferHeight() const override { return m_Data.FramebufferHeight; }
		VkExtent2D GetFramebufferExtent() const override { return { (uint32_t)m_Data.FramebufferWidth, (uint32_t)m_Data.FramebufferHeight }; }
		void* GetNativeWindow() override { return m_Window; }
	private:
		void Init(const WindowSpecs& specs);
		void Shutdown() const;
	private:
		GLFWwindow* m_Window;

		struct WindowData
		{
			std::string Title;
			int WindowWidth, WindowHeight;
			int FramebufferWidth, FramebufferHeight;
			bool FramebufferResize;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};

}
