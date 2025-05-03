#include "vulkanpch.h"
#include "VulkanRenderCommandBuffer.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanRenderer.h"

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

	VulkanRenderCommandBuffer::VulkanRenderCommandBuffer(VkCommandPool cmdPool, CommandBufferLevel cmdBufLevel, uint32_t queryCount)
		: m_CommandPool(cmdPool), m_CommandBufferLevel(cmdBufLevel), m_TimestampQueryBufferSize(queryCount << 1)
	{
		InvalidateCommandBuffers();
		if (queryCount > 0)
			InvalidateQueryPool();
	}

	void VulkanRenderCommandBuffer::InvalidateQueryPool()
	{
		auto device = VulkanContext::GetCurrentDevice();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		m_TimestampQueryPoolBuffer.resize(m_TimestampQueryBufferSize);
		m_TimestampQueryPools.resize(framesInFlight);

		// Creating Query Pool
		VkQueryPoolCreateInfo queryPoolCreateInfo{};
		queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolCreateInfo.queryCount = m_TimestampQueryBufferSize;
		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;

		VkCommandBuffer cmdBuffer = device->GetCommandBuffer();
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			vkCreateQueryPool(device->GetVulkanDevice(), &queryPoolCreateInfo, nullptr, &m_TimestampQueryPools[i]);
			vkCmdResetQueryPool(cmdBuffer, m_TimestampQueryPools[i], 0, m_TimestampQueryBufferSize);
		}

		device->FlushCommandBuffer(cmdBuffer);
	}

	void VulkanRenderCommandBuffer::InvalidateCommandBuffers()
	{
		auto device = VulkanContext::GetCurrentDevice();

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		m_CommandBuffers.resize(framesInFlight);

		// Allocating Command Buffers
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = Utils::VulkanCommandBufferLevel(m_CommandBufferLevel);
		allocInfo.commandPool = m_CommandPool;
		allocInfo.commandBufferCount = framesInFlight;

		VK_CHECK_RESULT(vkAllocateCommandBuffers(device->GetVulkanDevice(), &allocInfo, m_CommandBuffers.data()), "Failed to Allocate Command Buffers!");

		if (m_CommandBufferLevel == CommandBufferLevel::Primary)
			m_AllCommandBuffers.push_back(m_CommandBuffers);
	}

	VulkanRenderCommandBuffer::~VulkanRenderCommandBuffer()
	{
		auto device = VulkanContext::GetCurrentDevice();

		vkFreeCommandBuffers(device->GetVulkanDevice(), m_CommandPool,
			(uint32_t)m_CommandBuffers.size(), m_CommandBuffers.data());

		if (m_TimestampQueryBufferSize)
		{
			for (auto& queryPool : m_TimestampQueryPools)
				vkDestroyQueryPool(device->GetVulkanDevice(), queryPool, nullptr);
		}

		m_CommandBuffers.clear();
	}

	void VulkanRenderCommandBuffer::Begin()
	{
		Renderer::Submit([this]
		{
			VK_CORE_PROFILE_FN("VulkanRenderCommandBuffer::Begin");

			VkCommandBuffer commandBuffer = RT_GetActiveCommandBuffer();

			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;

			vkBeginCommandBuffer(commandBuffer, &beginInfo);

			if (m_TimestampQueryBufferSize)
			{
				RetrieveQueryPoolResults();

				VkQueryPool queryPool = RT_GetCurrentTimestampQueryPool();
				vkCmdResetQueryPool(commandBuffer, queryPool, 0, m_TimestampQueryBufferSize);
			}
		});
	}

	void VulkanRenderCommandBuffer::Begin(VkRenderPass renderPass, VkFramebuffer framebuffer)
	{
		VkCommandBuffer commandBuffer = RT_GetActiveCommandBuffer();
		VkQueryPool queryPool = RT_GetCurrentTimestampQueryPool();

		VkCommandBufferInheritanceInfo inheritanceInfo{};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.renderPass = renderPass;
		inheritanceInfo.framebuffer = framebuffer;

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		beginInfo.pInheritanceInfo = &inheritanceInfo;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		if (queryPool)
			vkCmdResetQueryPool(commandBuffer, queryPool, 0, m_TimestampQueryBufferSize);
	}

	void VulkanRenderCommandBuffer::End()
	{
		Renderer::Submit([this] { vkEndCommandBuffer(RT_GetActiveCommandBuffer()); });
	}

	void VulkanRenderCommandBuffer::Execute(VkCommandBuffer secondaryCmdBuffers[], uint32_t count)
	{
		vkCmdExecuteCommands(RT_GetActiveCommandBuffer(), count, secondaryCmdBuffers);
	}

	void VulkanRenderCommandBuffer::RetrieveQueryPoolResults()
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkQueryPool queryPool = RT_GetCurrentTimestampQueryPool();
		vkGetQueryPoolResults(device->GetVulkanDevice(),
			queryPool,
			0,
			m_TimestampQueryBufferSize, sizeof(uint64_t) * m_TimestampQueryBufferSize,
			(void*)m_TimestampQueryPoolBuffer.data(), sizeof(uint64_t),
			VK_QUERY_RESULT_64_BIT);
	}

	void VulkanRenderCommandBuffer::IncrementQueryIndex()
	{
		m_TimestampsQueryIndex += 2;
		m_TimestampsQueryIndex = m_TimestampsQueryIndex % m_TimestampQueryBufferSize;
	}

	uint64_t VulkanRenderCommandBuffer::GetQueryTime(uint32_t index) const
	{
		return m_TimestampQueryPoolBuffer[(index << 1) + 1] - m_TimestampQueryPoolBuffer[index << 1];
	}

	void VulkanRenderCommandBuffer::SubmitCommandBuffersToQueue()
	{
		Renderer::Submit([]
		{
			VK_CORE_PROFILE_FN("VulkanRenderer::SubmitCommandBuffersToQueue");

			uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
		
			std::vector<VkCommandBuffer> cmdBuffers;
			cmdBuffers.reserve(m_AllCommandBuffers.size());

			for (auto& cmdBuffer : m_AllCommandBuffers)
				cmdBuffers.emplace_back(cmdBuffer[frameIndex]);

			auto vulkanRenderer = VulkanRenderer::Get();
			vulkanRenderer->FinalQueueSubmit(cmdBuffers);
		});
	}

}
