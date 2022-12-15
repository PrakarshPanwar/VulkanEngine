#include "vulkanpch.h"
#include "VulkanRenderer.h"

#include "../Core/ImGuiLayer.h"
#include "../Core/Application.h"
#include "../Scene/SceneRenderer.h"
#include "Renderer.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDescriptor.h"

namespace VulkanCore {

	VulkanRenderer* VulkanRenderer::s_Instance;

	VulkanRenderer::VulkanRenderer(std::shared_ptr<WindowsWindow> window)
		: m_Window(window)
	{
		s_Instance = this;
		Init();
	}

	VulkanRenderer::~VulkanRenderer()
	{
		FreeQueryPool();
		FreeCommandBuffers();
	}

	void VulkanRenderer::Init()
	{
		RecreateSwapChain();
		CreateCommandBuffers();
		CreateQueryPool();
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

	VkCommandBuffer VulkanRenderer::BeginScene()
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		auto commandBuffer = SceneRenderer::GetSceneRenderer()->GetCommandBuffer(m_CurrentFrameIndex);

		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to Begin Recording Command Buffer!");
		return commandBuffer;
	}

	void VulkanRenderer::EndScene()
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

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

		VkCommandBufferInheritanceInfo inheritanceInfo{};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.renderPass = m_SwapChain->GetRenderPass();
		inheritanceInfo.framebuffer = m_SwapChain->GetFramebuffer(m_CurrentImageIndex);

		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

		VK_CHECK_RESULT(vkBeginCommandBuffer(m_SecondaryCommandBuffers[m_CurrentFrameIndex], &cmdBufInfo), "Failed to Begin Command Buffer!");

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_SwapChain->GetSwapChainExtent().width);
		viewport.height = static_cast<float>(m_SwapChain->GetSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{ {0, 0}, m_SwapChain->GetSwapChainExtent() };
		vkCmdSetViewport(m_SecondaryCommandBuffers[m_CurrentFrameIndex], 0, 1, &viewport);
		vkCmdSetScissor(m_SecondaryCommandBuffers[m_CurrentFrameIndex], 0, 1, &scissor);

		vkEndCommandBuffer(m_SecondaryCommandBuffers[m_CurrentFrameIndex]);

		m_ExecuteCommandBuffers[0] = m_SecondaryCommandBuffers[m_CurrentFrameIndex];
	}

	void VulkanRenderer::EndSwapChainRenderPass(VkCommandBuffer commandBuffer)
	{
		VK_CORE_ASSERT(IsFrameStarted, "Cannot call EndSwapChainRenderPass() if frame is not in progress!");
		VK_CORE_ASSERT(commandBuffer == GetCurrentCommandBuffer(), "Cannot end Render Pass on Command Buffer from a different frame!");
	
		m_ExecuteCommandBuffers[1] = ImGuiLayer::Get()->m_ImGuiCmdBuffers[Renderer::GetCurrentFrameIndex()];
		vkCmdExecuteCommands(commandBuffer, (uint32_t)m_ExecuteCommandBuffers.size(), m_ExecuteCommandBuffers.data());
		vkCmdEndRenderPass(commandBuffer);
	}

	void VulkanRenderer::BeginSceneRenderPass(VkCommandBuffer commandBuffer)
	{
		auto sceneRenderer = SceneRenderer::GetSceneRenderer();

		// TODO: This have to shifted in VulkanRenderCommandBuffer
		vkCmdResetQueryPool(commandBuffer, m_QueryPool, 0, 2);
		auto renderPass = sceneRenderer->GetRenderPass();
		Renderer::BeginRenderPass(renderPass);
	}

	void VulkanRenderer::EndSceneRenderPass(VkCommandBuffer commandBuffer)
	{
		auto renderPass = SceneRenderer::GetSceneRenderer()->GetRenderPass();
		Renderer::EndRenderPass(renderPass);
	}

	std::shared_ptr<VulkanTextureCube> VulkanRenderer::CreateEnviromentMap(const std::string& filepath)
	{
		constexpr uint32_t cubemapSize = 1024;

		std::shared_ptr<VulkanTexture> envEquirect = std::make_shared<VulkanTexture>(filepath);
		std::shared_ptr<VulkanTextureCube> envUnfiltered = std::make_shared<VulkanTextureCube>(cubemapSize, cubemapSize, ImageFormat::RGBA32F);
		envUnfiltered->Invalidate();

		auto equirectangularConversionShader = Renderer::GetShader("EquirectangularToCubeMap");
		std::shared_ptr<VulkanComputePipeline> equirectangularConversionPipeline = std::make_shared<VulkanComputePipeline>(equirectangularConversionShader);

		Renderer::Submit([equirectangularConversionPipeline, envEquirect, envUnfiltered]()
		{
			auto device = VulkanContext::GetCurrentDevice();
			auto vulkanDescriptorPool = Application::Get()->GetVulkanDescriptorPool();

			VkDescriptorSet equirectSet;
			VulkanDescriptorWriter equirectSetWriter(*equirectangularConversionPipeline->GetDescriptorSetLayout(), *vulkanDescriptorPool);

			VkDescriptorImageInfo cubeMapImageInfo = envUnfiltered->GetDescriptorImageInfo();
			equirectSetWriter.WriteImage(0, &cubeMapImageInfo);

			VkDescriptorImageInfo equirecTexInfo = envEquirect->GetDescriptorImageInfo();
			equirectSetWriter.WriteImage(1, &equirecTexInfo);

			equirectSetWriter.Build(equirectSet);

			VkCommandBuffer dispatchCmd = device->GetCommandBuffer();

			vkCmdBindDescriptorSets(dispatchCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
				equirectangularConversionPipeline->GetVulkanPipelineLayout(), 0, 1,
				&equirectSet, 0, nullptr);

			equirectangularConversionPipeline->Bind(dispatchCmd);
			equirectangularConversionPipeline->Dispatch(dispatchCmd, cubemapSize / 16, cubemapSize / 16, 6);

			device->FlushCommandBuffer(dispatchCmd);
		});

		return envUnfiltered;
	}

	void VulkanRenderer::CreateCommandBuffers()
	{
		auto device = VulkanContext::GetCurrentDevice();

		m_CommandBuffers.resize(VulkanSwapChain::MaxFramesInFlight);
		m_SecondaryCommandBuffers.resize(VulkanSwapChain::MaxFramesInFlight);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = device->GetCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

		VK_CHECK_RESULT(vkAllocateCommandBuffers(device->GetVulkanDevice(), &allocInfo, m_CommandBuffers.data()), "Failed to Allocate Command Buffers!");

		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocInfo.commandPool = device->GetRenderThreadCommandPool();
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device->GetVulkanDevice(), &allocInfo, m_SecondaryCommandBuffers.data()), "Failed to Allocate Secondary Command Buffers!");
	}

	void VulkanRenderer::FreeCommandBuffers()
	{
		auto device = VulkanContext::GetCurrentDevice();

		vkFreeCommandBuffers(device->GetVulkanDevice(), device->GetCommandPool(),
			static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());

		vkFreeCommandBuffers(device->GetVulkanDevice(), device->GetRenderThreadCommandPool(),
			static_cast<uint32_t>(m_SecondaryCommandBuffers.size()), m_SecondaryCommandBuffers.data());

		m_CommandBuffers.clear();
		m_SecondaryCommandBuffers.clear();
	}

	void VulkanRenderer::CreateQueryPool()
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkQueryPoolCreateInfo queryPoolInfo{};
		queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolInfo.queryCount = 2;
		queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		
		vkCreateQueryPool(device->GetVulkanDevice(), &queryPoolInfo, nullptr, &m_QueryPool);
	}

	void VulkanRenderer::FreeQueryPool()
	{
		vkDestroyQueryPool(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), m_QueryPool, nullptr);
	}

	void VulkanRenderer::RecreateSwapChain()
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto extent = m_Window->GetExtent();

		while (extent.width == 0 || extent.height == 0)
		{
			extent = m_Window->GetExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device->GetVulkanDevice());
		m_SwapChain.reset();

		if (m_SwapChain == nullptr)
		{
			m_SwapChain = std::make_unique<VulkanSwapChain>(extent);
		}

		else
		{
			std::shared_ptr<VulkanSwapChain> oldSwapChain = std::move(m_SwapChain);
			m_SwapChain = std::make_unique<VulkanSwapChain>(extent, oldSwapChain);

			if (!oldSwapChain->CompareSwapFormats(*m_SwapChain->GetSwapChain()))
			{
				VK_CORE_ASSERT(false, "Swap Chain Image(or Depth) Format has changed!");
			}
		}

	}

	void VulkanRenderer::FinalQueueSubmit()
	{
		auto sceneRenderer = SceneRenderer::GetSceneRenderer();

		const std::vector<VkCommandBuffer> cmdBuffers{ GetCurrentCommandBuffer(), sceneRenderer->GetCommandBuffer(m_CurrentFrameIndex)};
		auto result = m_SwapChain->SubmitCommandBuffers(cmdBuffers, &m_CurrentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Window->IsWindowResize())
		{
			m_Window->ResetWindowResizeFlag();
			RecreateSwapChain();
			sceneRenderer->RecreateScene();
		}

		else if (result != VK_SUCCESS)
			VK_CORE_ERROR("Failed to Present Swap Chain Image!");

		IsFrameStarted = false;
		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % VulkanSwapChain::MaxFramesInFlight;
	}

}