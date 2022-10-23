#pragma once
#include "Platform/Vulkan/VulkanRenderPass.h"

namespace VulkanCore {

	class Renderer
	{
	public:
		static void SetCommandBuffers(const std::vector<VkCommandBuffer>& cmdBuffers);
		static int GetCurrentFrameIndex();
		static void BeginRenderPass(std::shared_ptr<VulkanRenderPass> renderPass);
		static void EndRenderPass(std::shared_ptr<VulkanRenderPass> renderPass);
	private:
		static std::vector<VkCommandBuffer> m_CommandBuffers;
	};

}