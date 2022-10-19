#include "vulkanpch.h"
#include "VulkanSwapChain.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"
#include "VulkanAllocator.h"

namespace VulkanCore {

	VulkanSwapChain* VulkanSwapChain::s_Instance;

	VulkanSwapChain::VulkanSwapChain(VulkanDevice& vkDevice, VkExtent2D windowExtent)
		: m_VulkanDevice(vkDevice), m_WindowExtent(windowExtent)
	{
		s_Instance = this;
		Init();
	}

	VulkanSwapChain::VulkanSwapChain(VulkanDevice& vkDevice, VkExtent2D windowExtent, std::shared_ptr<VulkanSwapChain> prev)
		: m_VulkanDevice(vkDevice), m_WindowExtent(windowExtent), m_OldSwapChain(prev)
	{
		s_Instance = this;
		Init();

		m_OldSwapChain = nullptr;
	}

	VulkanSwapChain::~VulkanSwapChain()
	{
		for (int i = 0; i < m_ColorImages.size(); i++)
		{
			vkDestroyImageView(m_VulkanDevice.GetVulkanDevice(), m_ColorImageViews[i], nullptr);
			vmaDestroyImage(VulkanContext::GetVulkanMemoryAllocator(), m_ColorImages[i], m_ColorImageMemories[i]);
		}

		for (auto imageView : m_SwapChainImageViews)
			vkDestroyImageView(m_VulkanDevice.GetVulkanDevice(), imageView, nullptr);

		m_SwapChainImageViews.clear();

		if (m_SwapChain != nullptr)
		{
			vkDestroySwapchainKHR(m_VulkanDevice.GetVulkanDevice(), m_SwapChain, nullptr);
			m_SwapChain = nullptr;
		}


		for (int i = 0; i < m_DepthImages.size(); i++)
		{
			vkDestroyImageView(m_VulkanDevice.GetVulkanDevice(), m_DepthImageViews[i], nullptr);
			vkDestroyImage(m_VulkanDevice.GetVulkanDevice(), m_DepthImages[i], nullptr);
			vkFreeMemory(m_VulkanDevice.GetVulkanDevice(), m_DepthImageMemories[i], nullptr);
		}

		for (auto framebuffer : m_SwapChainFramebuffers)
			vkDestroyFramebuffer(m_VulkanDevice.GetVulkanDevice(), framebuffer, nullptr);

		vkDestroyRenderPass(m_VulkanDevice.GetVulkanDevice(), m_RenderPass, nullptr);

		// Cleanup Synchronization Objects
		for (size_t i = 0; i < MaxFramesInFlight; i++)
		{
			vkDestroySemaphore(m_VulkanDevice.GetVulkanDevice(), m_RenderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(m_VulkanDevice.GetVulkanDevice(), m_ImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(m_VulkanDevice.GetVulkanDevice(), m_InFlightFences[i], nullptr);
		}
	}

	VkFormat VulkanSwapChain::FindDepthFormat()
	{
		return m_VulkanDevice.FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	VkResult VulkanSwapChain::AcquireNextImage(uint32_t* imageIndex)
	{
		vkWaitForFences(m_VulkanDevice.GetVulkanDevice(), 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

		VkResult result = vkAcquireNextImageKHR(m_VulkanDevice.GetVulkanDevice(), m_SwapChain, std::numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, imageIndex);

		return result;
	}

	VkResult VulkanSwapChain::SubmitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex)
	{
		if (m_ImagesInFlight[*imageIndex] != VK_NULL_HANDLE)
			vkWaitForFences(m_VulkanDevice.GetVulkanDevice(), 1, &m_ImagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);

		m_ImagesInFlight[*imageIndex] = m_InFlightFences[m_CurrentFrame];

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = buffers;

		VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(m_VulkanDevice.GetVulkanDevice(), 1, &m_InFlightFences[m_CurrentFrame]);

		VK_CHECK_RESULT(vkQueueSubmit(m_VulkanDevice.GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]), "Failed to Submit Draw Command Buffer!");

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_SwapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = imageIndex;

		auto result = vkQueuePresentKHR(m_VulkanDevice.GetPresentQueue(), &presentInfo);

		m_CurrentFrame = (m_CurrentFrame + 1) % MaxFramesInFlight;

		return result;
	}

	VkResult VulkanSwapChain::SubmitCommandBuffers(const std::vector<VkCommandBuffer>& buffers, uint32_t* imageIndex)
	{
		if (m_ImagesInFlight[*imageIndex] != VK_NULL_HANDLE)
			vkWaitForFences(m_VulkanDevice.GetVulkanDevice(), 1, &m_ImagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);

		m_ImagesInFlight[*imageIndex] = m_InFlightFences[m_CurrentFrame];

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = static_cast<uint32_t>(buffers.size());
		submitInfo.pCommandBuffers = buffers.data();

		VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(m_VulkanDevice.GetVulkanDevice(), 1, &m_InFlightFences[m_CurrentFrame]);

		VK_CHECK_RESULT(vkQueueSubmit(m_VulkanDevice.GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]), "Failed to Submit Draw Command Buffer!");

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_SwapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = imageIndex;

		auto result = vkQueuePresentKHR(m_VulkanDevice.GetPresentQueue(), &presentInfo);

		m_CurrentFrame = (m_CurrentFrame + 1) % MaxFramesInFlight;

		return result;
	}

	bool VulkanSwapChain::CompareSwapFormats(const VulkanSwapChain& swapChain) const
	{
		return swapChain.m_SwapChainImageFormat == m_SwapChainImageFormat &&
			   swapChain.m_SwapChainDepthFormat == m_SwapChainDepthFormat;
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
		auto device = VulkanContext::GetCurrentDevice();
		SwapChainSupportDetails swapChainSupport = VulkanContext::GetCurrentContext()->QuerySwapChainSupport(device->GetPhysicalDevice());

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes);
		VkExtent2D extent = ChooseSwapExtent(swapChainSupport.Capabilities);

		uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
		if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)
			imageCount = swapChainSupport.Capabilities.maxImageCount;

		const auto vulkanSurface = VulkanContext::GetCurrentContext()->m_VkSurface;

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = vulkanSurface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = m_VulkanDevice.FindPhysicalQueueFamilies();
		uint32_t queueFamilyIndices[] = { indices.GraphicsFamily, indices.PresentFamily };

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

		VK_CHECK_RESULT(vkCreateSwapchainKHR(m_VulkanDevice.GetVulkanDevice(), &createInfo, nullptr, &m_SwapChain), "Failed to Create Swap Chain!");

		// We only specified a minimum number of Images in the SwapChain, so the implementation is
		// allowed to create a SwapChain with more. That's why we'll first query the final number of
		// images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
		// retrieve the handles.
		vkGetSwapchainImagesKHR(m_VulkanDevice.GetVulkanDevice(), m_SwapChain, &imageCount, nullptr);
		m_SwapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_VulkanDevice.GetVulkanDevice(), m_SwapChain, &imageCount, m_SwapChainImages.data());

		m_SwapChainImageFormat = surfaceFormat.format;
		m_SwapChainExtent = extent;
	}

	void VulkanSwapChain::CreateImageViews()
	{
		m_SwapChainImageViews.resize(m_SwapChainImages.size());

		for (size_t i = 0; i < m_SwapChainImages.size(); i++)
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_SwapChainImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = m_SwapChainImageFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VK_CHECK_RESULT(vkCreateImageView(m_VulkanDevice.GetVulkanDevice(), &viewInfo, nullptr, &m_SwapChainImageViews[i]), "Failed to Create Texture Image View!");
		}
	}

	void VulkanSwapChain::CreateColorResources()
	{
		VkFormat format = GetSwapChainImageFormat();
		VkExtent2D swapChainExtent = GetSwapChainExtent();
		VulkanAllocator allocator("SwapChainImages");

		m_ColorImages.resize(GetImageCount());
		m_ColorImageMemories.resize(GetImageCount());
		m_ColorImageViews.resize(GetImageCount());

		for (int i = 0; i < m_ColorImages.size(); i++)
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
			imageInfo.samples = m_VulkanDevice.GetMSAASampleCount();
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.flags = 0;

			// TODO: Use VMA method/allocator
			m_ColorImageMemories[i] = m_VulkanDevice.CreateImage(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_ColorImages[i]);

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

			VK_CHECK_RESULT(vkCreateImageView(m_VulkanDevice.GetVulkanDevice(), &viewInfo, nullptr, &m_ColorImageViews[i]), "Failed to Create Texture Image View!");
		}
	}

	void VulkanSwapChain::CreateDepthResources()
	{
		VkFormat depthFormat = FindDepthFormat();
		m_SwapChainDepthFormat = depthFormat;
		VkExtent2D swapChainExtent = GetSwapChainExtent();

		m_DepthImages.resize(GetImageCount());
		m_DepthImageMemories.resize(GetImageCount());
		m_DepthImageViews.resize(GetImageCount());

		for (int i = 0; i < m_DepthImages.size(); i++)
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
			imageInfo.samples = m_VulkanDevice.GetMSAASampleCount();
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.flags = 0;

			// TODO: Use VMA method/allocator
			m_VulkanDevice.CreateImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImages[i], m_DepthImageMemories[i]);

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

			VK_CHECK_RESULT(vkCreateImageView(m_VulkanDevice.GetVulkanDevice(), &viewInfo, nullptr, &m_DepthImageViews[i]), "Failed to Create Texture Image View!");
		}
	}

	void VulkanSwapChain::CreateRenderPass()
	{
		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = FindDepthFormat();
		depthAttachment.samples = m_VulkanDevice.GetMSAASampleCount();
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = GetSwapChainImageFormat();
		colorAttachment.samples = m_VulkanDevice.GetMSAASampleCount();
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachmentResolve;
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
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstSubpass = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VK_CHECK_RESULT(vkCreateRenderPass(m_VulkanDevice.GetVulkanDevice(), &renderPassInfo, nullptr, &m_RenderPass), "Failed to Create Render Pass!");
	}

	void VulkanSwapChain::CreateFramebuffers()
	{
		m_SwapChainFramebuffers.resize(GetImageCount());
		VkExtent2D swapChainExtent = GetSwapChainExtent();

		for (size_t i = 0; i < GetImageCount(); i++)
		{
			std::array<VkImageView, 3> attachments = { m_ColorImageViews[i], m_DepthImageViews[i], m_SwapChainImageViews[i]};

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			VK_CHECK_RESULT(vkCreateFramebuffer(m_VulkanDevice.GetVulkanDevice(), &framebufferInfo, nullptr, &m_SwapChainFramebuffers[i]), "Failed to Create Framebuffer!");
		}
	}

	void VulkanSwapChain::CreateSyncObjects()
	{
		m_ImageAvailableSemaphores.resize(MaxFramesInFlight);
		m_RenderFinishedSemaphores.resize(MaxFramesInFlight);
		m_InFlightFences.resize(MaxFramesInFlight);
		m_ImagesInFlight.resize(GetImageCount(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MaxFramesInFlight; i++)
		{
			VK_CORE_ASSERT(
				vkCreateSemaphore(m_VulkanDevice.GetVulkanDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) == VK_SUCCESS &&
				vkCreateSemaphore(m_VulkanDevice.GetVulkanDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) == VK_SUCCESS &&
				vkCreateFence(m_VulkanDevice.GetVulkanDevice(), &fenceInfo, nullptr, &m_InFlightFences[i]) == VK_SUCCESS, 
				"Failed to Create Synchronization Objects for a Frame!");
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

	VkPresentModeKHR VulkanSwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR)
			{
				VK_CORE_INFO("Present mode: V-Sync");
				return availablePresentMode;
			}
		}

		VK_CORE_INFO("Present mode: Mailbox");
		return VK_PRESENT_MODE_MAILBOX_KHR;
	}

	VkExtent2D VulkanSwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		
		VkExtent2D actualExtent = m_WindowExtent;
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}

}