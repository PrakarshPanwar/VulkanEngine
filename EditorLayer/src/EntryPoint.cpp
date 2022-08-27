#include "VulkanCore/Core/Application.h"
#include "EditorLayer.h"

namespace VulkanCore {

	class EditorApp : public Application
	{
	public:
		EditorApp()
		{
			PushLayer(new EditorLayer());
		}

		~EditorApp() = default;
	};

}

int main()
{
	std::unique_ptr<VulkanCore::Application> app = std::make_unique<VulkanCore::EditorApp>();
	app->Run();
}