#pragma once
#include "Platform/Vulkan/VulkanRenderPass.h"
#include "VulkanCore/Renderer/RenderThread.h"

#define USE_RENDER_THREAD 1

namespace VulkanCore {

	class Renderer
	{
	public:
		static void SetCommandBuffers(const std::vector<VkCommandBuffer>& cmdBuffers);
		static int GetCurrentFrameIndex();
		static void BeginRenderPass(std::shared_ptr<VulkanRenderPass> renderPass);
		static void EndRenderPass(std::shared_ptr<VulkanRenderPass> renderPass);

		template<typename FuncT>
		static void Submit(FuncT&& func)
		{
			RenderThread::SubmitToThread(func);
		}
	private:
		static std::vector<VkCommandBuffer> m_CommandBuffers;
	};

}