#pragma once

#include "Platform/Windows/WindowsWindow.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanRenderer.h"

#include "VulkanCore/Events/ApplicationEvent.h"

#include "ImGuiLayer.h"
#include "LayerStack.h"
#include "Timer.h"

// Required for ImGui
#define SUBMIT_TO_MAIN_THREAD VulkanCore::Application::Get()->SubmitToMainThread

namespace VulkanCore {

	struct ApplicationCommandLineArgs
	{
		int Count = 0;
		char** CmdArgs = nullptr;

		const char* operator[](int index) const
		{
			VK_CORE_ASSERT(index < Count, "Index is greater than Count!");
			return CmdArgs[index];
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
		
		void SubmitToMainThread(std::function<void()>&& func);

		Window* GetWindow() { return m_Window.get(); }
		WindowsWindow* GetWindowsWindow() { return std::dynamic_pointer_cast<WindowsWindow>(m_Window).get(); }
		std::shared_ptr<ImGuiLayer> GetImGuiLayer() { return m_ImGuiLayer; }
		static Application* Get() { return s_Instance; }

		const ApplicationSpecification& GetSpecification() const { return m_Specification; }
	private:
		bool OnWindowClose(WindowCloseEvent& window);
		bool OnWindowResize(WindowResizeEvent& window);

		void ExecuteMainThreadQueue();
	private:
		ApplicationSpecification m_Specification;
		std::shared_ptr<Window> m_Window;
		std::unique_ptr<VulkanContext> m_Context;
		std::unique_ptr<VulkanRenderer> m_Renderer;
		std::shared_ptr<ImGuiLayer> m_ImGuiLayer;
		std::unique_ptr<Timer> m_AppTimer;

		std::vector<std::function<void()>> m_MainThreadQueue;
		std::mutex m_MainThreadQueueMutex;

		bool m_Running = true, m_GammaCorrection = false;
		LayerStack m_LayerStack;

		static Application* s_Instance;

		friend void TerminateApplication(Application* app);
	};

	void TerminateApplication(Application* app);

}
