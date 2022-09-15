#pragma once
#include "glfw_vulkan.h"

#include "Platform/Windows/WindowsWindow.h"
#include "Platform/Vulkan/VulkanModel.h"
#include "Platform/Vulkan/VulkanDescriptor.h"

#include "VulkanCore/Scene/Scene.h"
#include "VulkanCore/Scene/Entity.h"
#include "VulkanCore/Events/ApplicationEvent.h"
#include "VulkanCore/Renderer/VulkanRenderer.h"
#include "VulkanCore/Renderer/EditorCamera.h"
#include "ImGuiLayer.h"
#include "LayerStack.h"

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
		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer.get(); }
		static Application* Get() { return s_Instance; }

		inline VulkanDescriptorPool* GetVulkanDescriptorPool() { return m_GlobalPool.get(); }
	private:
		bool OnWindowClose(WindowCloseEvent& window);
		bool OnWindowResize(WindowResizeEvent& window);
	private:
		std::shared_ptr<Window> m_Window;
		std::unique_ptr<VulkanDevice> m_VulkanDevice;
		std::unique_ptr<VulkanRenderer> m_Renderer;
		std::unique_ptr<VulkanDescriptorPool> m_GlobalPool;
		std::unique_ptr<ImGuiLayer> m_ImGuiLayer;
		bool m_Running = true, m_GammaCorrection = false;

		LayerStack m_LayerStack;

		static Application* s_Instance;
	};

}