#include "vulkanpch.h"
#include "Application.h"

#include "VulkanCore/Systems/RenderSystem.h"
#include "VulkanCore/Systems/PointLightSystem.h"
#include "Platform/Vulkan/VulkanBuffer.h"
#include "Platform/Vulkan/VulkanTexture.h"

#include "VulkanCore/Core/Log.h"
#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Camera.h"
#include <chrono>

#include "VulkanCore/Scene/Entity.h"
#include "imgui.h"

#include <numbers>
#include "imgui_impl_vulkan.h"
#include <filesystem>

namespace VulkanCore {

	Application* Application::s_Instance;

	std::shared_ptr<VulkanModel> CreateCubeModel(VulkanDevice& device, glm::vec3 offset)
	{
		ModelBuilder modelBuilder{};

		modelBuilder.Vertices = {

			// Left Face
			{ { -0.5f, -0.5f, -0.5f }, { 0.9f, 0.9f, 0.9f } },
			{ { -0.5f,  0.5f,  0.5f }, { 0.9f, 0.9f, 0.9f } },
			{ { -0.5f, -0.5f,  0.5f }, { 0.9f, 0.9f, 0.9f } },
			{ { -0.5f,  0.5f, -0.5f }, { 0.9f, 0.9f, 0.9f } },

			// Right Face (Yellow)
			{ {  0.5f, -0.5f, -0.5f }, { 0.8f, 0.8f, 0.1f } },
			{ {  0.5f,  0.5f,  0.5f }, { 0.8f, 0.8f, 0.1f } },
			{ {  0.5f, -0.5f,  0.5f }, { 0.8f, 0.8f, 0.1f } },
			{ {  0.5f,  0.5f, -0.5f }, { 0.8f, 0.8f, 0.1f } },

			// Top Face (Orange, Remember Y-Axis points down)
			{ { -0.5f, -0.5f, -0.5f }, { 0.9f, 0.6f, 0.1f } },
			{ {  0.5f, -0.5f,  0.5f }, { 0.9f, 0.6f, 0.1f } },
			{ { -0.5f, -0.5f,  0.5f }, { 0.9f, 0.6f, 0.1f } },
			{ {  0.5f, -0.5f, -0.5f }, { 0.9f, 0.6f, 0.1f } },

			// Bottom Face (Red)
			{ { -0.5f,  0.5f, -0.5f }, { 0.8f, 0.1f, 0.1f } },
			{ {  0.5f,  0.5f,  0.5f }, { 0.8f, 0.1f, 0.1f } },
			{ { -0.5f,  0.5f,  0.5f }, { 0.8f, 0.1f, 0.1f } },
			{ {  0.5f,  0.5f, -0.5f }, { 0.8f, 0.1f, 0.1f } },

			// Nose Face (Blue)
			{ { -0.5f, -0.5f,  0.5f }, { 0.1f, 0.1f, 0.8f } },
			{ {  0.5f,  0.5f,  0.5f }, { 0.1f, 0.1f, 0.8f } },
			{ { -0.5f,  0.5f,  0.5f }, { 0.1f, 0.1f, 0.8f } },
			{ {  0.5f, -0.5f,  0.5f }, { 0.1f, 0.1f, 0.8f } },

			// Tail Face (green)
			{ { -0.5f, -0.5f, -0.5f }, { 0.1f, 0.8f, 0.1f } },
			{ {  0.5f,  0.5f, -0.5f }, { 0.1f, 0.8f, 0.1f } },
			{ { -0.5f,  0.5f, -0.5f }, { 0.1f, 0.8f, 0.1f } },
			{ {  0.5f, -0.5f, -0.5f }, { 0.1f, 0.8f, 0.1f } }
		};

		for (auto& v : modelBuilder.Vertices)
			v.Position += offset;

		modelBuilder.Indices = { 0,  1,  2,  0,  3,  1,  4,  5,  6,  4,  7,  5,  8,  9,  10, 8,  11, 9,
						  12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21 };

		return std::make_shared<VulkanModel>(device, modelBuilder);
	}


	Application::Application()
	{
		s_Instance = this;
		Log::Init();

		std::filesystem::current_path("../VulkanCore");
		m_Window = std::make_shared<WindowsWindow>(WindowSpecs(1280, 720, "Vulkan Application"));
		m_Window->SetEventCallback(VK_CORE_BIND_EVENT_FN(Application::OnEvent));

		Init();
	}

	Application::~Application()
	{
		vkDeviceWaitIdle(m_VulkanDevice->GetVulkanDevice());
		m_ImGuiLayer->ShutDown();
	}

	void Application::Init()
	{
		m_VulkanDevice = std::make_unique<VulkanDevice>(*std::dynamic_pointer_cast<WindowsWindow>(m_Window));
		m_Renderer = std::make_unique<VulkanRenderer>(*std::dynamic_pointer_cast<WindowsWindow>(m_Window), *m_VulkanDevice);

		DescriptorPoolBuilder descriptorPoolBuilder = DescriptorPoolBuilder(*m_VulkanDevice);
		descriptorPoolBuilder.SetMaxSets(VulkanSwapChain::MaxFramesInFlight).AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VulkanSwapChain::MaxFramesInFlight);
		descriptorPoolBuilder.SetMaxSets(VulkanSwapChain::MaxFramesInFlight).AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VulkanSwapChain::MaxFramesInFlight);
		m_GlobalPool = descriptorPoolBuilder.Build();

		m_ImGuiLayer = std::make_unique<ImGuiLayer>();
		m_ImGuiLayer->OnAttach();
	}

	void Application::Run()
	{
		while (m_Running)
		{
			m_Window->OnUpdate();

			if (auto commandBuffer = m_Renderer->BeginFrame())
			{
				m_Renderer->BeginSwapChainRenderPass(commandBuffer);

				for (Layer* layer : m_LayerStack)
					layer->OnUpdate();

				m_ImGuiLayer->ImGuiBegin();
				for (Layer* layer : m_LayerStack)
					layer->OnImGuiRender();
				m_ImGuiLayer->ImGuiRenderandEnd(commandBuffer);

				m_Renderer->EndSwapChainRenderPass(commandBuffer);
				m_Renderer->EndFrame();
			}
		}
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(VK_CORE_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(VK_CORE_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			(*--it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	bool Application::OnWindowClose(WindowCloseEvent& window)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& window)
	{
		m_Renderer->RecreateSwapChain();
		return true;
	}

}