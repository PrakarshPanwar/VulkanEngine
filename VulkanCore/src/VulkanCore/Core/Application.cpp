#include "vulkanpch.h"
#include "Application.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "optick.h"

namespace VulkanCore {

	Application* Application::s_Instance;

	Application::Application(const ApplicationSpecification& spec)
		: m_Specification(spec)
	{
		s_Instance = this;

		m_AppTimer = std::make_unique<Timer>("Application Initialization");
		Log::Init();

		std::filesystem::current_path(m_Specification.WorkingDirectory);
		m_Window = std::make_shared<WindowsWindow>(WindowSpecs(1920, 1080, m_Specification.Name));
		m_Window->SetEventCallback(VK_CORE_BIND_EVENT_FN(Application::OnEvent));

		Init();
	}

	Application::~Application()
	{
		vkDeviceWaitIdle(VulkanContext::GetCurrentDevice()->GetVulkanDevice());
		m_ImGuiLayer->ShutDown();
		Renderer::DestroyShaders();
	}

	void Application::Init()
	{
		m_Context = std::make_unique<VulkanContext>(std::dynamic_pointer_cast<WindowsWindow>(m_Window));
		m_Renderer = std::make_unique<VulkanRenderer>(std::dynamic_pointer_cast<WindowsWindow>(m_Window));

		DescriptorPoolBuilder descriptorPoolBuilder = DescriptorPoolBuilder();
		descriptorPoolBuilder.SetMaxSets(100).AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10);
		descriptorPoolBuilder.SetMaxSets(1000).AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10);
		m_GlobalPool = descriptorPoolBuilder.Build();

		m_ImGuiLayer = std::make_shared<ImGuiLayer>();
		m_ImGuiLayer->OnAttach();

		Renderer::Init();
		Renderer::BuildShaders();
		Renderer::SetRendererAPI(m_Renderer.get());
	}

	void Application::Run()
	{
		m_AppTimer.reset();

		while (m_Running)
		{
			VK_CORE_BEGIN_FRAME("Main Thread");
			m_Window->OnUpdate();

			// Render Swapchain/ImGui
			m_Renderer->BeginFrame();
			m_Renderer->BeginSwapChainRenderPass();

			m_ImGuiLayer->ImGuiBegin();
			Renderer::Submit([this]() { RenderImGui(); });
			Renderer::Submit([this]() { m_ImGuiLayer->ImGuiEnd(); });

			m_Renderer->EndSwapChainRenderPass();
			m_Renderer->EndFrame();

			// Render Scene
			m_Renderer->BeginScene();
			for (Layer* layer : m_LayerStack)
				layer->OnUpdate();

			m_Renderer->EndScene();

			m_Renderer->FinalQueueSubmit();
		}

		RenderThread::WaitAndDestroy();
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

	void Application::RenderImGui()
	{
		VK_CORE_PROFILE();

		m_ImGuiLayer->ImGuiNewFrame();
		for (Layer* layer : m_LayerStack)
			layer->OnImGuiRender();
	}

}