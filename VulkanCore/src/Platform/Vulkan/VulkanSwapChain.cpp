#include "vulkanpch.h"
#include "VulkanSwapChain.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "VulkanAllocator.h"

namespace VulkanCore {

	namespace Utils {

		static const char* VulkanPresentMode(VkPresentModeKHR presentMode)
		{
			switch (presentMode)
			{
			case VK_PRESENT_MODE_IMMEDIATE_KHR:			return "Immediate";
			case VK_PRESENT_MODE_MAILBOX_KHR:			return "Mailbox";
			case VK_PRESENT_MODE_FIFO_KHR:				return "FIFO(V-Sync)";
			case VK_PRESENT_MODE_FIFO_RELAXED_KHR:		return "FIFO(Relaxed)";
			case VK_PRESENT_MODE_FIFO_LATEST_READY_EXT: return "FIFO(Latest)";
			default:
				VK_CORE_ASSERT(false, "Present Mode is undefined!");
				return "Unknown";
			}
		}

	}

	VulkanSwapChain* VulkanSwapChain::s_Instance;

	VulkanSwapChain::VulkanSwapChain(VkExtent2D windowExtent)
		: m_WindowExtent(windowExtent)
	{
		s_Instance = this;
		Init();
	}

	VulkanSwapChain::VulkanSwapChain(VkExtent2D windowExtent, std::shared_ptr<VulkanSwapChain> prev)
		: m_WindowExtent(windowExtent), m_OldSwapChain(prev)
	{
		s_Instance = this;
		Init();

		m_OldSwapChain = nullptr;
	}

	VulkanSwapChain::~VulkanSwapChain()
	{
		auto device = VulkanContext::GetCurrentDevice();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		VulkanAllocator allocator("DestroySwapChain");

		for (size_t i = 0; i < framesInFlight; ++i)
		{
			// Cleanup Synchronization Objects
			vkDestroySemaphore(device->GetVulkanDevice(), m_ImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device->GetVulkanDevice(), m_InFlightFences[i], nullptr);
		}

		for (size_t i = 0; i < GetImageCount(); ++i)
		{
			// Cleanup Images
			vkDestroyImageView(device->GetVulkanDevice(), m_SCImageViews[i], nullptr);
			vkDestroyImageView(device->GetVulkanDevice(), m_ColorImageViews[i], nullptr);
			vkDestroyImageView(device->GetVulkanDevice(), m_DepthImageViews[i], nullptr);

			allocator.DestroyImage(m_ColorImages[i], m_ColorImageAllocations[i]);
			allocator.DestroyImage(m_DepthImages[i], m_DepthImageAllocations[i]);

			// Cleanup Framebuffers
			vkDestroyFramebuffer(device->GetVulkanDevice(), m_SCFramebuffers[i], nullptr);

			// Cleanup Presentation Semaphores
			vkDestroySemaphore(device->GetVulkanDevice(), m_ReadyToPresentSemaphores[i], nullptr);
		}

		// Cleanup Render Pass
		vkDestroyRenderPass(device->GetVulkanDevice(), m_SCRenderPass, nullptr);

		// Cleanup Swap Chain
		if (m_SwapChain)
		{
			vkDestroySwapchainKHR(device->GetVulkanDevice(), m_SwapChain, nullptr);
			m_SwapChain = nullptr;
		}
	}

	VkFormat VulkanSwapChain::FindDepthFormat()
	{
		return VulkanContext::GetCurrentDevice()->FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	VkResult VulkanSwapChain::AcquireNextImage(uint32_t* imageIndex)
	{
		const auto device = VulkanContext::GetCurrentDevice();
		vkWaitForFences(device->GetVulkanDevice(), 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

		return vkAcquireNextImageKHR(device->GetVulkanDevice(), m_SwapChain, std::numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, imageIndex);
	}

	VkResult VulkanSwapChain::SubmitCommandBuffers(const std::vector<VkCommandBuffer>& buffers, uint32_t* imageIndex)
	{
		const auto device = VulkanContext::GetCurrentDevice();
		uint32_t currentImageIndex = *imageIndex;

		if (m_ImagesInFlight[currentImageIndex] != VK_NULL_HANDLE)
			vkWaitForFences(device->GetVulkanDevice(), 1, &m_ImagesInFlight[currentImageIndex], VK_TRUE, UINT64_MAX);

		m_ImagesInFlight[currentImageIndex] = m_InFlightFences[m_CurrentFrame];

		VkSemaphoreSubmitInfo waitSemaphoreInfo{};
		waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitSemaphoreInfo.semaphore = m_ImageAvailableSemaphores[m_CurrentFrame];
		waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSemaphoreSubmitInfo signalSemaphoreInfo{};
		signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalSemaphoreInfo.semaphore = m_ReadyToPresentSemaphores[currentImageIndex];
		signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

		std::vector<VkCommandBufferSubmitInfo> commandBufferInfos(buffers.size());
		for (uint32_t i = 0; auto& vulkanCmdBuffer : buffers)
		{
			VkCommandBufferSubmitInfo commandBufferInfo{};
			commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR;
			commandBufferInfo.commandBuffer = vulkanCmdBuffer;
			
			commandBufferInfos[i] = commandBufferInfo;
			++i;
		}

		VkSubmitInfo2 submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.waitSemaphoreInfoCount = 1;
		submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
		submitInfo.signalSemaphoreInfoCount = 1;
		submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;
		submitInfo.commandBufferInfoCount = static_cast<uint32_t>(commandBufferInfos.size());
		submitInfo.pCommandBufferInfos = commandBufferInfos.data();

		vkResetFences(device->GetVulkanDevice(), 1, &m_InFlightFences[m_CurrentFrame]);

		glm::vec4 queueLabelColor = { 0.1f, 0.3f, 0.5f, 1.0f };
		VKUtils::SetQueueLabel(device->GetGraphicsQueue(), "Graphics-Queue", &queueLabelColor.x);

		VK_CHECK_RESULT(vkQueueSubmit2(device->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]), "Failed to Submit Draw Command Buffer!");

		VKUtils::EndQueueLabel(device->GetGraphicsQueue());

		VkSemaphore signalSemaphores[] = { m_ReadyToPresentSemaphores[currentImageIndex] };

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_SwapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = imageIndex;

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		m_CurrentFrame = (m_CurrentFrame + 1) % framesInFlight;

		return vkQueuePresentKHR(device->GetPresentQueue(), &presentInfo);
	}

	bool VulkanSwapChain::CompareSwapFormats(const VulkanSwapChain& swapChain) const
	{
		return swapChain.m_SCImageFormat == m_SCImageFormat &&
			swapChain.m_SCDepthFormat == m_SCDepthFormat;
	}

	void VulkanSwapChain::Init()
	{
		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateColorResources();
		CreateDepthResources();
		CreateFramebuffers();
		CreateSyncObjects();
	}

	void VulkanSwapChain::CreateSwapChain()
	{
		auto context = VulkanContext::GetCurrentContext();
		auto device = VulkanContext::GetCurrentDevice();
		SwapChainSupportDetails swapChainSupport = context->QuerySwapChainSupport(device->GetPhysicalDevice());

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes, VK_PRESENT_MODE_FIFO_KHR);
		VkExtent2D extent = ChooseSwapExtent(swapChainSupport.Capabilities);

		uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
		if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)
			imageCount = swapChainSupport.Capabilities.maxImageCount;

		QueueFamilyIndices indices = device->FindPhysicalQueueFamilies();
		uint32_t queueFamilyIndices[] = { indices.GraphicsFamily, indices.PresentFamily };

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = context->m_VulkanSurface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		if (indices.GraphicsFamily != indices.PresentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;      // Optional
			createInfo.pQueueFamilyIndices = nullptr;  // Optional
		}

		createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = m_OldSwapChain == nullptr ? VK_NULL_HANDLE : m_OldSwapChain->m_SwapChain;

		VK_CHECK_RESULT(vkCreateSwapchainKHR(device->GetVulkanDevice(), &createInfo, nullptr, &m_SwapChain), "Failed to Create Swap Chain!");

		// We only specified a minimum number of Images in the SwapChain, so the implementation is allowed
		// to create a SwapChain with more. That's why we'll first query the final number of images with
		// vkGetSwapchainImagesKHR, then resize the container and finally call it again to retrieve the handles.
		vkGetSwapchainImagesKHR(device->GetVulkanDevice(), m_SwapChain, &imageCount, nullptr);
		m_SCImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device->GetVulkanDevice(), m_SwapChain, &imageCount, m_SCImages.data());

		m_SCImageFormat = surfaceFormat.format;
		m_SCExtent = extent;
	}

	void VulkanSwapChain::CreateImageViews()
	{
		auto device = VulkanContext::GetCurrentDevice();

		m_SCImageViews.resize(m_SCImages.size());

		for (size_t i = 0; i < m_SCImages.size(); ++i)
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_SCImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = m_SCImageFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VK_CHECK_RESULT(vkCreateImageView(device->GetVulkanDevice(), &viewInfo, nullptr, &m_SCImageViews[i]), "Failed to Create Texture Image View!");
		}
	}

	void VulkanSwapChain::CreateColorResources()
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkFormat format = GetSwapChainImageFormat();
		VkExtent2D swapChainExtent = GetSwapChainExtent();
		VulkanAllocator allocator("SwapChainColorImages");

		m_ColorImages.resize(GetImageCount());
		m_ColorImageAllocations.resize(GetImageCount());
		m_ColorImageViews.resize(GetImageCount());

		for (int i = 0; i < m_ColorImages.size(); ++i)
		{
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = swapChainExtent.width;
			imageInfo.extent.height = swapChainExtent.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = format;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage =  VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			imageInfo.samples = device->GetMSAASampleCount();
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.flags = 0;

			m_ColorImageAllocations[i] = allocator.AllocateImage(imageInfo, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, m_ColorImages[i]);
			VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(), VK_OBJECT_TYPE_IMAGE, std::format("SwapChainColor: {}", i), m_ColorImages[i]);

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_ColorImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = format;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VK_CHECK_RESULT(vkCreateImageView(device->GetVulkanDevice(), &viewInfo, nullptr, &m_ColorImageViews[i]), "Failed to Create Texture Image View!");
		}
	}

	void VulkanSwapChain::CreateDepthResources()
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkFormat depthFormat = FindDepthFormat();
		m_SCDepthFormat = depthFormat;
		VkExtent2D swapChainExtent = GetSwapChainExtent();
		VulkanAllocator allocator("SwapChainDepthImages");

		m_DepthImages.resize(GetImageCount());
		m_DepthImageAllocations.resize(GetImageCount());
		m_DepthImageViews.resize(GetImageCount());

		for (int i = 0; i < m_DepthImages.size(); ++i)
		{
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = swapChainExtent.width;
			imageInfo.extent.height = swapChainExtent.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = depthFormat;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageInfo.samples = device->GetMSAASampleCount();
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.flags = 0;

			m_DepthImageAllocations[i] = allocator.AllocateImage(imageInfo, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, m_DepthImages[i]);
			VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(), VK_OBJECT_TYPE_IMAGE, std::format("SwapChainDepth: {}", i), m_DepthImages[i]);

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_DepthImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = depthFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VK_CHECK_RESULT(vkCreateImageView(device->GetVulkanDevice(), &viewInfo, nullptr, &m_DepthImageViews[i]), "Failed to Create Texture Image View!");
		}
	}

	void VulkanSwapChain::CreateRenderPass()
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = GetSwapChainImageFormat();
		colorAttachment.samples = device->GetMSAASampleCount();
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = FindDepthFormat();
		depthAttachment.samples = device->GetMSAASampleCount();
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachmentResolve{};
		colorAttachmentResolve.format = GetSwapChainImageFormat();
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		colorAttachmentResolve.flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;

		VkAttachmentReference colorAttachmentResolveRef{};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstSubpass = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VK_CHECK_RESULT(vkCreateRenderPass(device->GetVulkanDevice(), &renderPassInfo, nullptr, &m_SCRenderPass), "Failed to Create Render Pass!");
	}

	void VulkanSwapChain::CreateFramebuffers()
	{
		auto device = VulkanContext::GetCurrentDevice();

		m_SCFramebuffers.resize(GetImageCount());
		VkExtent2D swapChainExtent = GetSwapChainExtent();

		for (size_t i = 0; i < GetImageCount(); ++i)
		{
			std::array attachments = { m_ColorImageViews[i], m_DepthImageViews[i], m_SCImageViews[i] };

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_SCRenderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			VK_CHECK_RESULT(vkCreateFramebuffer(device->GetVulkanDevice(), &framebufferInfo, nullptr, &m_SCFramebuffers[i]), "Failed to Create Framebuffer!");
		}
	}

	void VulkanSwapChain::CreateSyncObjects()
	{
		auto device = VulkanContext::GetCurrentDevice();

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		m_ImageAvailableSemaphores.resize(framesInFlight);
		m_ReadyToPresentSemaphores.resize(GetImageCount());
		m_InFlightFences.resize(framesInFlight);
		m_ImagesInFlight.resize(GetImageCount(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		// Acquire Image Semaphores(Frames in Flight)
		for (size_t i = 0; i < framesInFlight; ++i)
		{
			VK_CORE_ASSERT(vkCreateSemaphore(device->GetVulkanDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) == VK_SUCCESS &&
				vkCreateFence(device->GetVulkanDevice(), &fenceInfo, nullptr, &m_InFlightFences[i]) == VK_SUCCESS, 
				"Failed to Create Synchronization Objects for a Frame!");

			VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(), VK_OBJECT_TYPE_SEMAPHORE, std::format("ImageAvailableSemaphore: {}", i), m_ImageAvailableSemaphores[i]);
		}

		// Presentation Semaphores(SwapChain Image Count)
		for (size_t i = 0; i < GetImageCount(); ++i)
		{
			VK_CORE_ASSERT(vkCreateSemaphore(device->GetVulkanDevice(), &semaphoreInfo, nullptr, &m_ReadyToPresentSemaphores[i]) == VK_SUCCESS,
				"Failed to Create Synchronization Objects for a Frame!");

			VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(), VK_OBJECT_TYPE_SEMAPHORE, std::format("RenderFinishedSemaphore: {}", i), m_ReadyToPresentSemaphores[i]);
		}
	}

	VkSurfaceFormatKHR VulkanSwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT)
				return availableFormat;
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return availableFormat;
		}

		return availableFormats[0];
	}

	VkPresentModeKHR VulkanSwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, VkPresentModeKHR requiredPresentMode)
	{
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == requiredPresentMode)
			{
				VK_CORE_INFO("Present Mode: {}", Utils::VulkanPresentMode(availablePresentMode));
				return availablePresentMode;
			}
		}

		VK_CORE_INFO("Present Mode: {}", Utils::VulkanPresentMode(VK_PRESENT_MODE_FIFO_KHR));
		return VK_PRESENT_MODE_FIFO_KHR; // Required by all implementations, so we return it as a fallback.
	}

	VkExtent2D VulkanSwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return capabilities.currentExtent;
		
		VkExtent2D actualExtent = m_WindowExtent;
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}

}
