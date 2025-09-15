#pragma once
#include "VulkanCore/Core/glfw_vulkan.h"
#include "VulkanCore/Events/Event.h"

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

	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() = default;

		virtual void OnUpdate() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual VkExtent2D GetExtent() const = 0;
		virtual const std::string& GetWindowName() const = 0;

		virtual bool IsWindowResize() const = 0;
		virtual void ResetWindowResizeFlag() = 0;

		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void* GetNativeWindow() { return nullptr; }

		static std::shared_ptr<Window> Create(const WindowSpecs& specs = WindowSpecs());
	};

}
