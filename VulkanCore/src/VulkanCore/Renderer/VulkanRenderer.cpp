#include "vulkanpch.h"
#include "VulkanRenderer.h"

#include "../Core/ImGuiLayer.h"
#include "../Scene/SceneRenderer.h"

namespace VulkanCore {

	VulkanRenderer* VulkanRenderer::s_Instance;

	VulkanRenderer::VulkanRenderer(WindowsWindow& appwindow, VulkanDevice& device)
		: m_Window(appwindow), m_VulkanDevice(device)
	{
		s_Instance = this;
		Init();
	}

	VulkanRenderer::~VulkanRenderer()
	{
		FreeCommandBuffers();
	}

	void VulkanRenderer::Init()
	{
		RecreateSwapChain();
		CreateCommandBuffers();
	}

	VkCommandBuffer VulkanRenderer::BeginFrame()
	{
		VK_CORE_ASSERT(!IsFrameStarted, "Cannot call BeginFrame() while frame being already in progress!");

		auto result = m_SwapChain->AcquireNextImage(&m_CurrentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapChain();
			return nullptr;
		}

		VK_CORE_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to Acquire Swap Chain!");

		IsFrameStarted = true;

		auto commandBuffer = GetCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to Begin Recording Command Buffer!");

		return commandBuffer;
	}

	void VulkanRenderer::EndFrame()
	{
		VK_CORE_ASSERT(IsFrameStarted, "Cannot call EndFrame() while frame is not in progress!");

		auto commandBuffer = GetCurrentCommandBuffer();
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer), "Failed to Record Command Buffer!");
	}

	VkCommandBuffer VulkanRenderer::BeginSceneFrame()
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		auto commandBuffer = SceneRenderer::GetSceneRenderer()->GetCommandBuffer(m_CurrentFrameIndex);

		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to Begin Recording Command Buffer!");
		return commandBuffer;
	}

	void VulkanRenderer::EndSceneFrame()
	{
		auto commandBuffer = SceneRenderer::GetSceneRenderer()->GetCommandBuffer(m_CurrentFrameIndex);
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer), "Failed to Record Command Buffer!");
	}

	void VulkanRenderer::BeginSwapChainRenderPass(VkCommandBuffer commandBuffer)
	{
		VK_CORE_ASSERT(IsFrameStarted, "Cannot call BeginSwapChainRenderPass() if frame is not in progress!");
		VK_CORE_ASSERT(commandBuffer == GetCurrentCommandBuffer(), "Cannot begin Render Pass on Command Buffer from a different frame!");
	
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_SwapChain->GetRenderPass();
		renderPassInfo.framebuffer = m_SwapChain->GetFramebuffer(m_CurrentImageIndex);
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_SwapChain->GetSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = (uint32_t)clearValues.size();
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_SwapChain->GetSwapChainExtent().width);
		viewport.height = static_cast<float>(m_SwapChain->GetSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{ {0, 0}, m_SwapChain->GetSwapChainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void VulkanRenderer::EndSwapChainRenderPass(VkCommandBuffer commandBuffer)
	{
		VK_CORE_ASSERT(IsFrameStarted, "Cannot call EndSwapChainRenderPass() if frame is not in progress!");
		VK_CORE_ASSERT(commandBuffer == GetCurrentCommandBuffer(), "Cannot end Render Pass on Command Buffer from a different frame!");
	
		vkCmdEndRenderPass(commandBuffer);
	}

	void VulkanRenderer::BeginSceneRenderPass(VkCommandBuffer commandBuffer)
	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = SceneRenderer::GetSceneRenderer()->GetRenderPass();
		renderPassInfo.framebuffer = SceneRenderer::GetSceneRenderer()->GetFramebuffer(m_CurrentImageIndex);
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_SwapChain->GetSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = (uint32_t)clearValues.size();
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_SwapChain->GetSwapChainExtent().width);
		viewport.height = static_cast<float>(m_SwapChain->GetSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{ {0, 0}, m_SwapChain->GetSwapChainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void VulkanRenderer::EndSceneRenderPass(VkCommandBuffer commandBuffer)
	{
		vkCmdEndRenderPass(commandBuffer);
	}

	void VulkanRenderer::CreateCommandBuffers()
	{
		m_CommandBuffers.resize(VulkanSwapChain::MaxFramesInFlight);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_VulkanDevice.GetCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

		VK_CHECK_RESULT(vkAllocateCommandBuffers(m_VulkanDevice.GetVulkanDevice(), &allocInfo, m_CommandBuffers.data()), "Failed to Allocate Command Buffers!");
	}

	void VulkanRenderer::FreeCommandBuffers()
	{
		vkFreeCommandBuffers(m_VulkanDevice.GetVulkanDevice(), m_VulkanDevice.GetCommandPool(),
			static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());

		m_CommandBuffers.clear();
	}

	void VulkanRenderer::RecreateSwapChain()
	{
		auto extent = m_Window.GetExtent();

		while (extent.width == 0 || extent.height == 0)
		{
			extent = m_Window.GetExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_VulkanDevice.GetVulkanDevice());
		m_SwapChain.reset();

		if (m_SwapChain == nullptr)
		{
			m_SwapChain = std::make_unique<VulkanSwapChain>(m_VulkanDevice, extent);
		}

		else
		{
			std::shared_ptr<VulkanSwapChain> oldSwapChain = std::move(m_SwapChain);
			m_SwapChain = std::make_unique<VulkanSwapChain>(m_VulkanDevice, extent, oldSwapChain);

			if (!oldSwapChain->CompareSwapFormats(*m_SwapChain->GetSwapChain()))
			{
				VK_CORE_ASSERT(false, "Swap Chain Image(or Depth) Format has changed!");
			}
		}

	}

	void VulkanRenderer::FinalQueueSubmit()
	{
		const std::vector<VkCommandBuffer> cmdBuffers{ GetCurrentCommandBuffer(), SceneRenderer::GetSceneRenderer()->GetCommandBuffer(m_CurrentFrameIndex) };

		auto result = m_SwapChain->SubmitCommandBuffers(cmdBuffers, &m_CurrentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Window.IsWindowResize())
		{
			m_Window.ResetWindowResizeFlag();
			RecreateSwapChain();
			SceneRenderer::GetSceneRenderer()->RecreateScene();
		}

		else if (result != VK_SUCCESS)
			VK_CORE_ERROR("Failed to Present Swap Chain Image!");

		IsFrameStarted = false;
		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % VulkanSwapChain::MaxFramesInFlight;
	}

}