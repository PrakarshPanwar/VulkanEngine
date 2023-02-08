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
		VulkanRenderCommandBuffer(VkCommandPool cmdPool, CommandBufferLevel cmdBufLevel = CommandBufferLevel::Primary);
		~VulkanRenderCommandBuffer();

		void Begin();
		void Begin(VkRenderPass renderPass, VkFramebuffer framebuffer);
		void End();
		void End(VkCommandBuffer secondaryCmdBuffers[], uint32_t count);

		inline VkCommandBuffer GetActiveCommandBuffer() const;

		static void SubmitCommandBuffersToQueue();
	private:
		VkCommandPool m_CommandPool = nullptr;
		CommandBufferLevel m_CommandBufferLevel;
		std::vector<VkCommandBuffer> m_CommandBuffers;

		static std::vector<std::vector<VkCommandBuffer>> m_AllCommandBuffers;
	};

}
