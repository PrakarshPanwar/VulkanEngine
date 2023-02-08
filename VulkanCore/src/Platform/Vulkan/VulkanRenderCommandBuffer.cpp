#include "vulkanpch.h"
#include "VulkanRenderCommandBuffer.h"
#include "VulkanSwapChain.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "VulkanCore/Renderer/VulkanRenderer.h"

namespace VulkanCore {

	namespace Utils {

		static VkCommandBufferLevel VulkanCommandBufferLevel(CommandBufferLevel cmdLevel)
		{
			switch (cmdLevel)
			{
			case CommandBufferLevel::Primary:   return VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			case CommandBufferLevel::Secondary: return VK_COMMAND_BUFFER_LEVEL_SECONDARY;
			default:
				VK_CORE_ASSERT(false, "Command Buffer Level undefined!");
				return VK_COMMAND_BUFFER_LEVEL_MAX_ENUM;
			}
		}

	}

	std::vector<std::vector<VkCommandBuffer>> VulkanRenderCommandBuffer::m_AllCommandBuffers;

	VulkanRenderCommandBuffer::VulkanRenderCommandBuffer(VkCommandPool cmdPool, CommandBufferLevel cmdBufLevel)
		: m_CommandPool(cmdPool), m_CommandBufferLevel(cmdBufLevel)
	{
		auto device = VulkanContext::GetCurrentDevice();

		uint32_t framesInFlight = VulkanSwapChain::MaxFramesInFlight;
		m_CommandBuffers.resize(framesInFlight);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = Utils::VulkanCommandBufferLevel(cmdBufLevel);
		allocInfo.commandPool = cmdPool;
		allocInfo.commandBufferCount = framesInFlight;

		VK_CHECK_RESULT(vkAllocateCommandBuffers(device->GetVulkanDevice(), &allocInfo, m_CommandBuffers.data()), "Failed to Allocate Command Buffers!");

		if (cmdBufLevel == CommandBufferLevel::Primary)
			m_AllCommandBuffers.push_back(m_CommandBuffers);
	}

	VulkanRenderCommandBuffer::~VulkanRenderCommandBuffer()
	{
		auto device = VulkanContext::GetCurrentDevice();

		vkFreeCommandBuffers(device->GetVulkanDevice(), m_CommandPool,
			(uint32_t)m_CommandBuffers.size(), m_CommandBuffers.data());

		m_CommandBuffers.clear();
	}

	void VulkanRenderCommandBuffer::Begin()
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		vkBeginCommandBuffer(GetActiveCommandBuffer(), &beginInfo);
	}

	void VulkanRenderCommandBuffer::Begin(VkRenderPass renderPass, VkFramebuffer framebuffer)
	{
		VkCommandBufferInheritanceInfo inheritanceInfo{};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.renderPass = renderPass;
		inheritanceInfo.framebuffer = framebuffer;

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		beginInfo.pInheritanceInfo = &inheritanceInfo;

		vkBeginCommandBuffer(GetActiveCommandBuffer(), &beginInfo);
	}

	void VulkanRenderCommandBuffer::End()
	{
		vkEndCommandBuffer(GetActiveCommandBuffer());
	}

	void VulkanRenderCommandBuffer::End(VkCommandBuffer secondaryCmdBuffers[], uint32_t count)
	{
		VkCommandBuffer commandBuffer = GetActiveCommandBuffer();

		vkCmdExecuteCommands(commandBuffer, count, secondaryCmdBuffers);
		vkEndCommandBuffer(commandBuffer);
	}

	VkCommandBuffer VulkanRenderCommandBuffer::GetActiveCommandBuffer() const
	{
		return m_CommandBuffers[Renderer::GetCurrentFrameIndex()];
	}

	void VulkanRenderCommandBuffer::SubmitCommandBuffersToQueue()
	{
		uint32_t currentFrameIndex = Renderer::GetCurrentFrameIndex();
		
		std::vector<VkCommandBuffer> cmdBuffers;
		cmdBuffers.reserve(m_AllCommandBuffers.size());

		for (auto& cmdBuffer : m_AllCommandBuffers)
			cmdBuffers.emplace_back(cmdBuffer[currentFrameIndex]);

		auto vulkanRenderer = VulkanRenderer::Get();
		vulkanRenderer->FinalQueueSubmit(cmdBuffers);
	}

}
