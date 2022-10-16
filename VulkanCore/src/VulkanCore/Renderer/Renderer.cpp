#include "vulkanpch.h"
#include "Renderer.h"

namespace VulkanCore {

	void Renderer::BeginRenderPass(VkCommandBuffer beginPassCmd, std::shared_ptr<VulkanRenderPass> renderPass)
	{
		//vkCmdBeginRenderPass(beginPassCmd, )
	}

	void Renderer::EndRenderPass(VkCommandBuffer endPassCmd, std::shared_ptr<VulkanRenderPass> renderPass)
	{
		//vkCmdEndRenderPass(endPassCmd, )
	}

}