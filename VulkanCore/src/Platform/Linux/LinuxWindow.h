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

		const std::string& GetWindowName() const override { return m_WindowSpecs.Name; }
		bool IsWindowResized() const override { return m_WindowSpecs.FramebufferResize; }
		void ResetWindowResizeFlag() override { m_WindowSpecs.FramebufferResize = false; };
		void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; };

		uint32_t GetWidth() const override { return m_WindowSpecs.Width; }
		uint32_t GetHeight() const override { return m_WindowSpecs.Height; }
		VkExtent2D GetExtent() const override { return { (uint32_t)m_WindowSpecs.Width, (uint32_t)m_WindowSpecs.Height }; }
		void* GetNativeWindow() override { return m_Window; }
	private:
		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

		void Init(const WindowSpecs& specs);
		void Shutdown() const;
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
