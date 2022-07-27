#include "vulkanpch.h"
#include "Log.h"

#include "VulkanApplication.h"

int main()
{
	::VulkanCore::Log::Init();

	std::unique_ptr<VulkanCore::VulkanApplication> app = std::make_unique<VulkanCore::VulkanApplication>();
	app->Run();
}