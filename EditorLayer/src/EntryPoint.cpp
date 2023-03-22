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

	std::unique_ptr<Application> CreateApplication(ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec{};
		spec.Name = "Vulkan Application";
		spec.WorkingDirectory = "../VulkanCore";
		spec.Fullscreen = true;
		spec.CommandLineArgs = args;

		return std::make_unique<EditorApp>(spec);
	}

}

int main(int argc, char** argv)
{
	std::unique_ptr<VulkanCore::Application> app = VulkanCore::CreateApplication({ argc, argv });
	app->Run();
}