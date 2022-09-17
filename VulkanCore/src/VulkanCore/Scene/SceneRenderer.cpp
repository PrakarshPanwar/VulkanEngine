#include "vulkanpch.h"
#include "SceneRenderer.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

#include "Platform/Vulkan/VulkanSwapChain.h"
#include "Platform/Vulkan/VulkanTexture.h"

namespace VulkanCore {

	SceneRenderer* SceneRenderer::s_Instance = nullptr;

	SceneRenderer::SceneRenderer()
	{
		s_Instance = this;
		Init();
	}

	SceneRenderer::~SceneRenderer()
	{
		Release();
	}

	void SceneRenderer::Init()
	{
		CreateCommandBuffers();
		CreateImagesandViews();
		CreateRenderPass();
		CreateFramebuffers();
	}

	void SceneRenderer::CreateImagesandViews()
	{
		auto device = VulkanDevice::GetDevice();
		auto swapChain = VulkanSwapChain::GetSwapChain();

		VkFormat depthfmt = swapChain->FindDepthFormat();

		m_SceneImages.resize(swapChain->GetImageCount());
		m_SceneColorImages.resize(swapChain->GetImageCount());
		m_SceneDepthImages.resize(swapChain->GetImageCount());

		m_ImageAllocs.resize(swapChain->GetImageCount());
		m_ColorImageAllocs.resize(swapChain->GetImageCount());
		m_DepthImageAllocs.resize(swapChain->GetImageCount());

		m_SceneImageViews.resize(swapChain->GetImageCount());
		m_SceneColorImageViews.resize(swapChain->GetImageCount());
		m_SceneDepthImageViews.resize(swapChain->GetImageCount());

		for (int i = 0; i < swapChain->GetImageCount(); i++)
		{
			// For Resolve Images of Framebuffer
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.format = swapChain->GetSwapChainImageFormat();
			imageInfo.extent.width = swapChain->GetSwapChainExtent().width;
			imageInfo.extent.height = swapChain->GetSwapChainExtent().height;
			imageInfo.extent.depth = 1;
			imageInfo.arrayLayers = 1;

			uint32_t mipLevels = static_cast<uint32_t>(std::_Floor_of_log_2(std::max(
				swapChain->GetSwapChainExtent().width,
				swapChain->GetSwapChainExtent().height))) + 1;

			imageInfo.mipLevels = mipLevels;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			m_ImageAllocs[i] = device->CreateImage(imageInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_SceneImages[i]);

			VkCommandBuffer cmdBuffer = device->BeginSingleTimeCommands();

			Utils::InsertImageMemoryBarrier(cmdBuffer, m_SceneImages[i],
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

			device->EndSingleTimeCommands(cmdBuffer);

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.image = m_SceneImages[i];
			viewInfo.format = swapChain->GetSwapChainImageFormat();
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VK_CHECK_RESULT(vkCreateImageView(device->GetVulkanDevice(), &viewInfo, nullptr, &m_SceneImageViews[i]), "Failed to Create Scene Image Views!");

			// For Multisampled Framebuffer Images
			imageInfo.mipLevels = 1;
			imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			imageInfo.samples = device->GetMSAASampleCount();

			m_ColorImageAllocs[i] = device->CreateImage(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_SceneColorImages[i]);

			viewInfo.image = m_SceneColorImages[i];

			VK_CHECK_RESULT(vkCreateImageView(device->GetVulkanDevice(), &viewInfo, nullptr, &m_SceneColorImageViews[i]), "Failed to Create Scene Color Image Views!");

			// For Depth Resources
			imageInfo.format = depthfmt;
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

			m_DepthImageAllocs[i] = device->CreateImage(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_SceneDepthImages[i]);

			viewInfo.image = m_SceneDepthImages[i];
			viewInfo.format = depthfmt;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			VK_CHECK_RESULT(vkCreateImageView(device->GetVulkanDevice(), &viewInfo, nullptr, &m_SceneDepthImageViews[i]), "Failed to Create Scene Depth Image Views!");
		}
	}

	void SceneRenderer::CreateRenderPass()
	{
		auto device = VulkanDevice::GetDevice();
		auto swapChain = VulkanSwapChain::GetSwapChain();

		VkAttachmentDescription colorAttachmentResolve = {};
		colorAttachmentResolve.format = VK_FORMAT_B8G8R8A8_SRGB;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		colorAttachmentResolve.flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
		colorAttachment.samples = device->GetMSAASampleCount();
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = swapChain->FindDepthFormat();
		depthAttachment.samples = device->GetMSAASampleCount();
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentResolveRef = {};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstSubpass = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VK_CHECK_RESULT(vkCreateRenderPass(device->GetVulkanDevice(), &renderPassInfo, nullptr, &m_SceneRenderPass), "Failed to Create Scene Render Pass!");
	}

	void SceneRenderer::CreateFramebuffers()
	{
		auto device = VulkanDevice::GetDevice();
		auto swapChain = VulkanSwapChain::GetSwapChain();

		m_SceneFramebuffers.resize(swapChain->GetImageCount());

		for (int i = 0; i < swapChain->GetImageCount(); i++)
		{
			std::array<VkImageView, 3> attachments = { m_SceneColorImageViews[i], m_SceneDepthImageViews[i], m_SceneImageViews[i] };

			VkExtent2D swapChainExtent = swapChain->GetSwapChainExtent();

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_SceneRenderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			VK_CHECK_RESULT(vkCreateFramebuffer(device->GetVulkanDevice(), &framebufferInfo, nullptr, &m_SceneFramebuffers[i]), "Failed to Allocate Scene Framebuffers!");
		}
	}

	void SceneRenderer::Release()
	{
		auto device = VulkanDevice::GetDevice();
		for (int i = 0; i < m_SceneImages.size(); i++)
		{
			vkDestroyImageView(device->GetVulkanDevice(), m_SceneImageViews[i], nullptr);
			vmaDestroyImage(device->GetVulkanAllocator(), m_SceneImages[i], m_ImageAllocs[i]);
		}

		for (int i = 0; i < m_SceneDepthImages.size(); i++)
		{
			vkDestroyImageView(device->GetVulkanDevice(), m_SceneDepthImageViews[i], nullptr);
			vmaDestroyImage(device->GetVulkanAllocator(), m_SceneDepthImages[i], m_DepthImageAllocs[i]);
		}

		for (int i = 0; i < m_SceneColorImages.size(); i++)
		{
			vkDestroyImageView(device->GetVulkanDevice(), m_SceneColorImageViews[i], nullptr);
			vmaDestroyImage(device->GetVulkanAllocator(), m_SceneColorImages[i], m_ColorImageAllocs[i]);
		}

		for (auto& Framebuffer : m_SceneFramebuffers)
			vkDestroyFramebuffer(device->GetVulkanDevice(), Framebuffer, nullptr);

		vkDestroyRenderPass(device->GetVulkanDevice(), m_SceneRenderPass, nullptr);
	}

	void SceneRenderer::CreateCommandBuffers()
	{
		auto device = VulkanDevice::GetDevice();
		auto swapChain = VulkanSwapChain::GetSwapChain();

		m_SceneCommandBuffers.resize(swapChain->GetImageCount());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = device->GetCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(m_SceneCommandBuffers.size());

		VK_CHECK_RESULT(vkAllocateCommandBuffers(device->GetVulkanDevice(), &allocInfo, m_SceneCommandBuffers.data()), "Failed to Allocate Scene Command Buffers!");
	}

}