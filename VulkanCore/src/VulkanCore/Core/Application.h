#pragma once
#include "glfw_vulkan.h"

#include "Platform/Windows/WindowsWindow.h"
#include "Platform/Vulkan/VulkanMesh.h"
#include "Platform/Vulkan/VulkanDescriptor.h"
#include "Platform/Vulkan/VulkanContext.h"

#include "VulkanCore/Scene/Scene.h"
#include "VulkanCore/Scene/Entity.h"
#include "VulkanCore/Events/ApplicationEvent.h"
#include "VulkanCore/Renderer/VulkanRenderer.h"
#include "VulkanCore/Renderer/EditorCamera.h"

#include "ImGuiLayer.h"
#include "LayerStack.h"
#include "Timer.h"

namespace VulkanCore {

	class Application
	{
	public:
		Application();
		virtual ~Application();

		void Init();
		void Run();

		void OnEvent(Event& e);
		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		Window* GetWindow() { return m_Window.get(); }
		WindowsWindow* GetWindowsWindow() { return std::dynamic_pointer_cast<WindowsWindow>(m_Window).get(); }
		std::shared_ptr<ImGuiLayer> GetImGuiLayer() { return m_ImGuiLayer; }
		static Application* Get() { return s_Instance; }

		inline VulkanDescriptorPool* GetVulkanDescriptorPool() { return m_GlobalPool.get(); }
	private:
		bool OnWindowClose(WindowCloseEvent& window);
		bool OnWindowResize(WindowResizeEvent& window);
	private:
		std::shared_ptr<Window> m_Window;
		std::unique_ptr<VulkanContext> m_Context;
		std::unique_ptr<VulkanRenderer> m_Renderer;
		std::unique_ptr<VulkanDescriptorPool> m_GlobalPool;
		std::shared_ptr<ImGuiLayer> m_ImGuiLayer;
		std::unique_ptr<Timer> m_AppTimer;

		bool m_Running = true, m_GammaCorrection = false;
		LayerStack m_LayerStack;

		static Application* s_Instance;
	};

}