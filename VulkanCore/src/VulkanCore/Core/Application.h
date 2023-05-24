#pragma once

#include "Platform/Windows/WindowsWindow.h"
#include "Platform/Vulkan/VulkanDescriptor.h"
#include "Platform/Vulkan/VulkanContext.h"

#include "VulkanCore/Events/ApplicationEvent.h"
#include "VulkanCore/Renderer/VulkanRenderer.h"

#include "ImGuiLayer.h"
#include "LayerStack.h"
#include "Timer.h"

namespace VulkanCore {

	struct ApplicationCommandLineArgs
	{
		int Count = 0;
		char** Args = nullptr;

		const char* operator[](int index) const
		{
			VK_CORE_ASSERT(index < Count, "Index is greater than Count!");
			return Args[index];
		}
	};

	struct ApplicationSpecification
	{
		std::string Name = "VulkanCore Application";
		std::string WorkingDirectory;
		bool Fullscreen = false;
		ApplicationCommandLineArgs CommandLineArgs;
	};

	class Application
	{
	public:
		Application(const ApplicationSpecification& spec);
		virtual ~Application();

		void Init();
		void Run();

		void RenderImGui();

		void OnEvent(Event& e);
		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		Window* GetWindow() { return m_Window.get(); }
		WindowsWindow* GetWindowsWindow() { return std::dynamic_pointer_cast<WindowsWindow>(m_Window).get(); }
		std::shared_ptr<ImGuiLayer> GetImGuiLayer() { return m_ImGuiLayer; }
		static Application* Get() { return s_Instance; }

		inline const ApplicationSpecification& GetSpecification() const { return m_Specification; }
		inline VulkanDescriptorPool* GetVulkanDescriptorPool() { return m_GlobalPool.get(); }
		inline std::shared_ptr<VulkanDescriptorPool> GetDescriptorPool() { return m_GlobalPool; }
	private:
		bool OnWindowClose(WindowCloseEvent& window);
		bool OnWindowResize(WindowResizeEvent& window);
	private:
		ApplicationSpecification m_Specification;
		std::shared_ptr<Window> m_Window;
		std::unique_ptr<VulkanContext> m_Context;
		std::unique_ptr<VulkanRenderer> m_Renderer;
		std::shared_ptr<VulkanDescriptorPool> m_GlobalPool;
		std::shared_ptr<ImGuiLayer> m_ImGuiLayer;
		std::unique_ptr<Timer> m_AppTimer;

		bool m_Running = true, m_GammaCorrection = false;
		LayerStack m_LayerStack;

		static Application* s_Instance;
	};

}