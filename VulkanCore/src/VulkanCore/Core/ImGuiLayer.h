#pragma once
#include "glfw_vulkan.h"
#include "Platform/Vulkan/VulkanDescriptor.h"
#include "VulkanCore/Core/Layer.h"

namespace VulkanCore {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void OnAttach() override;
		void OnDetach() override;
		void ImGuiBegin();
		void ImGuiRenderandEnd(VkCommandBuffer commandBuffer);
		void ShutDown();

		static void CheckVkResult(VkResult error);
	private:
		void SetDarkThemeColor();
	private:
		std::unique_ptr<VulkanDescriptorPool> m_GlobalPool;
	};

}