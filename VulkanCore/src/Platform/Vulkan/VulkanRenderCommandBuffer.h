#pragma once
#include "VulkanContext.h"
#include "VulkanCore/Renderer/RenderCommandBuffer.h"
#include "VulkanCore/Renderer/Renderer.h"

namespace VulkanCore {

	enum class CommandBufferLevel
	{
		Primary,
		Secondary
	};

	class VulkanRenderCommandBuffer : public RenderCommandBuffer
	{
	public:
		VulkanRenderCommandBuffer(VkCommandPool cmdPool, CommandBufferLevel cmdBufLevel = CommandBufferLevel::Primary, uint32_t queryCount = 0);
		~VulkanRenderCommandBuffer();

		void Begin() override;
		void Begin(VkRenderPass renderPass, VkFramebuffer framebuffer);
		void End() override;
		void Execute(VkCommandBuffer secondaryCmdBuffers[], uint32_t count);

		VkCommandBuffer GetActiveCommandBuffer() const { return m_CommandBuffers[Renderer::GetCurrentFrameIndex()]; }
		VkCommandBuffer RT_GetActiveCommandBuffer() const { return m_CommandBuffers[Renderer::RT_GetCurrentFrameIndex()]; }
		VkQueryPool RT_GetCurrentTimestampQueryPool() const { return m_TimestampQueryPools[Renderer::RT_GetCurrentFrameIndex()]; }

		void RetrieveQueryPoolResults();
		void IncrementQueryIndex();
		uint64_t GetQueryTime(uint32_t index) const;
		uint32_t GetTimestampQueryIndex() const { return m_TimestampsQueryIndex; }

		static void SubmitCommandBuffersToQueue();
	private:
		void InvalidateCommandBuffers();
		void InvalidateQueryPool();
	private:
		VkCommandPool m_CommandPool = nullptr;
		CommandBufferLevel m_CommandBufferLevel;
		std::vector<VkCommandBuffer> m_CommandBuffers;

		std::vector<VkQueryPool> m_TimestampQueryPools;
		uint32_t m_TimestampQueryBufferSize = 0, m_TimestampsQueryIndex = 0;
		std::vector<uint64_t> m_TimestampQueryPoolBuffer;

		static std::vector<std::vector<VkCommandBuffer>> m_AllCommandBuffers;
	};

}
