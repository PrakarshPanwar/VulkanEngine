#pragma once
#include "VulkanCore/Core/glfw_vulkan.h"
#include "VulkanCore/Events/Event.h"

namespace VulkanCore {

	struct WindowSpecs
	{
		int Width, Height;
		std::string Name{};

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

		virtual const std::string& GetWindowName() const = 0;
		virtual uint32_t GetWindowWidth() const = 0;
		virtual uint32_t GetWindowHeight() const = 0;
		virtual VkExtent2D GetWindowExtent() const = 0;

		virtual uint32_t GetFramebufferWidth() const = 0;
		virtual uint32_t GetFramebufferHeight() const = 0;
		virtual VkExtent2D GetFramebufferExtent() const = 0;

		virtual bool IsWindowResized() const = 0;
		virtual void ResetResizeFlag() = 0;

		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void* GetNativeWindow() { return nullptr; }

		static std::shared_ptr<Window> Create(const WindowSpecs& specs = WindowSpecs());
	};

}
