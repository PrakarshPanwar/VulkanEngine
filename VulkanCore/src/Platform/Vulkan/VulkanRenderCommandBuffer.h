#pragma once
#include "VulkanContext.h"

namespace VulkanCore {

	enum class CommandBufferLevel
	{
		Primary,
		Secondary
	};

	class VulkanRenderCommandBuffer
	{
	public:
		VulkanRenderCommandBuffer(VkCommandPool cmdPool, CommandBufferLevel cmdBufLevel = CommandBufferLevel::Primary, uint32_t queryCount = 0);
		~VulkanRenderCommandBuffer();

		void Begin();
		void Begin(VkRenderPass renderPass, VkFramebuffer framebuffer);
		void End();
		void Execute(VkCommandBuffer secondaryCmdBuffers[], uint32_t count);

		inline VkCommandBuffer GetActiveCommandBuffer() const;

		void RetrieveQueryPoolResults();
		uint64_t GetQueryTime(uint32_t index);

		static void SubmitCommandBuffersToQueue();
	private:
		void InvalidateCommandBuffers();
		void InvalidateQueryPool();
	private:
		VkCommandPool m_CommandPool = nullptr;
		CommandBufferLevel m_CommandBufferLevel;
		std::vector<VkCommandBuffer> m_CommandBuffers;

		VkQueryPool m_TimestampQueryPool = nullptr;
		uint32_t m_TimestampQueryBufferSize = 0, m_TimestampsQueryIndex = 0;
		std::vector<uint64_t> m_TimestampQueryPoolBuffer;

		static std::vector<std::vector<VkCommandBuffer>> m_AllCommandBuffers;

		friend class Renderer;
	};

}
