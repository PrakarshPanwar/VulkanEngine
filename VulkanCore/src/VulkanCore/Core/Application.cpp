#include "vulkanpch.h"
#include "Application.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Renderer.h"

namespace VulkanCore {

	Application* Application::s_Instance;

	Application::Application(const ApplicationSpecification& spec)
		: m_Specification(spec)
	{
		s_Instance = this;

		m_AppTimer = std::make_unique<Timer>("Application Initialization");
		Log::Init();

		std::filesystem::current_path(m_Specification.WorkingDirectory);
		m_Window = Window::Create({ 1920, 1080, m_Specification.Name });
		m_Window->SetEventCallback(VK_CORE_BIND_EVENT_FN(Application::OnEvent));

		Init();
	}

	Application::~Application()
	{
		RenderThread::WaitAndDestroy();
		vkDeviceWaitIdle(VulkanContext::GetCurrentDevice()->GetVulkanDevice());
		m_ImGuiLayer->ShutDown();
	}

	void Application::Init()
	{
		m_Context = std::make_unique<VulkanContext>(m_Window);
		m_Renderer = std::make_unique<VulkanRenderer>(m_Window);

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
			VK_CORE_START_FRAME("Main Thread");
			m_Window->OnUpdate();

			ExecuteMainThreadQueue();

			// Render Swapchain/ImGui
			m_Renderer->BeginFrame();
			m_Renderer->BeginSCRenderPass();

			Renderer::Submit([this] { RenderImGui(); });
			Renderer::Submit([this] { m_ImGuiLayer->ImGuiEnd(); });

			m_Renderer->EndSCRenderPass();
			m_Renderer->EndFrame();

			for (Layer* layer : m_LayerStack)
				layer->OnUpdate();

			m_Renderer->WaitForRenderThread();
			VK_CORE_END_FRAME("Main Thread");
		}
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(VK_CORE_BIND_EVENT_FN(Application::OnWindowClose));
		//dispatcher.Dispatch<WindowResizeEvent>(VK_CORE_BIND_EVENT_FN(Application::OnWindowResize));

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

	void Application::SubmitToMainThread(std::function<void()>&& func)
	{
		std::scoped_lock submitLock(m_MainThreadQueueMutex);
		m_MainThreadQueue.emplace_back(func);
	}

	bool Application::OnWindowClose(WindowCloseEvent& window)
	{
		m_Running = false;
		return true;
	}

	void Application::ExecuteMainThreadQueue()
	{
		VK_CORE_PROFILE_FN("Application::ExecuteMainThreadQueue", TracyZoneLabelColor::Purple);

		std::scoped_lock executeLock(m_MainThreadQueueMutex);

		for (auto&& func : m_MainThreadQueue)
			func();

		m_MainThreadQueue.clear();
	}

	void Application::RenderImGui()
	{
		VK_CORE_PROFILE();

		m_ImGuiLayer->ImGuiBegin();
		for (Layer* layer : m_LayerStack)
			layer->OnImGuiRender();
	}

	void TerminateApplication(Application* app)
	{
		std::unique_ptr<VulkanContext> context = std::move(app->m_Context);
		std::unique_ptr<VulkanRenderer> renderer = std::move(app->m_Renderer);
		delete app;

		Renderer::ShutDown();
	}

}
