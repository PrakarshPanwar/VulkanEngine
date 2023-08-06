#include "VulkanCore/Core/Application.h"
#include "EditorLayer.h"

namespace VulkanCore {

	class EditorApp : public Application
	{
	public:
		EditorApp(const ApplicationSpecification& spec)
			: Application(spec)
		{
			PushLayer(new EditorLayer());
		}

		~EditorApp() = default;
	};

	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec{};
		spec.Name = "Vulkan Application";
		spec.WorkingDirectory = "../VulkanCore";
		spec.Fullscreen = true;
		spec.RayTracing = true;
		spec.CommandLineArgs = args;

		return new EditorApp(spec);
	}

}

int main(int argc, char** argv)
{
	VulkanCore::Application* app = VulkanCore::CreateApplication({ argc, argv });
	app->Run();

	TerminateApplication(app);
}