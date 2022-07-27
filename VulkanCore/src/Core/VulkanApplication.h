#pragma once
#include "glfw_vulkan.h"

#include "Platform/Windows/WindowsWindow.h"
#include "VulkanModel.h"
#include "Renderer/VulkanRenderer.h"
#include "VulkanGameObject.h"
#include "VulkanDescriptor.h"

#include "Scene/Scene.h"
#include "Events/ApplicationEvent.h"
#include "Renderer/EditorCamera.h"
#include "ImGuiLayer.h"

namespace VulkanCore {

	class VulkanApplication
	{
	public:
		VulkanApplication();
		~VulkanApplication();

		void Init();
		void Run();

		void OnEvent(Event& e);

		WindowsWindow* GetWindow() { return m_Window.get(); }
		static VulkanApplication* Get() { return s_Instance; }

		inline VulkanDescriptorPool* GetVulkanDescriptorPool() { return m_GlobalPool.get(); }
	private:
		void LoadGameObjects();
		void LoadEntities();

		bool OnWindowClose(WindowCloseEvent& window);
		bool OnWindowResize(WindowResizeEvent& window);
	private:
		std::unique_ptr<WindowsWindow> m_Window;
		std::unique_ptr<VulkanDevice> m_VulkanDevice;
		std::unique_ptr<VulkanRenderer> m_Renderer;
		std::unique_ptr<VulkanDescriptorPool> m_GlobalPool;
		std::unique_ptr<ImGuiLayer> m_ImGuiLayer;
		std::shared_ptr<Scene> m_Scene;
		EditorCamera m_EditorCamera;
		VulkanGameObject::Map m_GameObjects;
		bool m_Running = true;

		static VulkanApplication* s_Instance;
	};

}