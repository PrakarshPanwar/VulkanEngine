#pragma once
#include "Platform/Vulkan/VulkanRenderPass.h"

namespace VulkanCore {

	class Renderer
	{
	public:
		static void BeginRenderPass(VkCommandBuffer beginCmd, std::shared_ptr<VulkanRenderPass> renderPass);
		static void EndRenderPass(VkCommandBuffer beginCmd, std::shared_ptr<VulkanRenderPass> renderPass);
	private:
		static VkCommandBuffer m_CommandBuffer;
	};

}