#pragma once
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

		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void* GetNativeWindow() { return nullptr; }

		static Window* Create(const WindowSpecs& specs = WindowSpecs());
	};

}
