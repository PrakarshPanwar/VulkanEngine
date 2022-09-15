#include "vulkanpch.h"
#include "ImGuiLayer.h"

#include "Platform/Vulkan/VulkanSwapChain.h"
#include "Platform/Vulkan/VulkanTexture.h"
#include "Application.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <glm/gtx/log_base.hpp>
#include <filesystem>

#define IMGUI_VIEWPORTS 0
#define IMGUI_SEPARATE_RESOURCES 1

namespace VulkanCore {

	ImGuiLayer* ImGuiLayer::s_Instance;

	ImGuiLayer::ImGuiLayer()
	{
		s_Instance = this;
#if IMGUI_SEPARATE_RESOURCES
		Init();
#endif
	}

	void ImGuiLayer::Init()
	{
		CreateCommandBuffers();
		CreateImGuiViewportImages();
		CreateImGuiViewportImageViews();
#if MULTISAMPLING
		CreateImGuiMultisampleImages();
#endif
#if DEPTH_RESOURCES
		CreateDepthResources();
#endif
		CreateImGuiRenderPass();
		CreateImGuiFramebuffers();
	}

	ImGuiLayer::~ImGuiLayer()
	{
		auto device = VulkanDevice::GetDevice();
#if IMGUI_SEPARATE_RESOURCES
		for (int i = 0; i < m_ViewportImages.size(); i++)
		{
			vkDestroyImageView(device->GetVulkanDevice(), m_ViewportImageViews[i], nullptr);
			vmaDestroyImage(device->GetVulkanAllocator(), m_ViewportImages[i], m_ViewportImageAllocs[i]);
		}

#if DEPTH_RESOURCES
		for (int i = 0; i < m_DepthImages.size(); i++)
		{
			vkDestroyImageView(device->GetVulkanDevice(), m_DepthImageViews[i], nullptr);
			vmaDestroyImage(device->GetVulkanAllocator(), m_DepthImages[i], m_DepthImageAllocs[i]);
		}
#endif

#if MULTISAMPLING
		for (int i = 0; i < m_ColorImages.size(); i++)
		{
			vkDestroyImageView(device->GetVulkanDevice(), m_ColorImageViews[i], nullptr);
			vmaDestroyImage(device->GetVulkanAllocator(), m_ColorImages[i], m_ColorImageAllocs[i]);
		}
#endif

		for (auto& Framebuffer : m_Framebuffers)
			vkDestroyFramebuffer(device->GetVulkanDevice(), Framebuffer, nullptr);

		vkDestroyRenderPass(device->GetVulkanDevice(), m_ViewportRenderPass, nullptr);
#endif
	}

	void ImGuiLayer::OnAttach()
	{
		DescriptorPoolBuilder descriptorPoolBuilder = DescriptorPoolBuilder(*VulkanDevice::GetDevice());
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000);

		m_GlobalPool = descriptorPoolBuilder.Build();

		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#if IMGUI_VIEWPORTS
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif

		GLFWwindow* window = (GLFWwindow*)Application::Get()->GetWindow()->GetNativeWindow();
		ImGui_ImplGlfw_InitForVulkan(window, true);

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = VulkanDevice::GetDevice()->GetVulkanInstance();
		init_info.PhysicalDevice = VulkanDevice::GetDevice()->GetPhysicalDevice();
		init_info.Device = VulkanDevice::GetDevice()->GetVulkanDevice();
		init_info.Queue = VulkanDevice::GetDevice()->GetGraphicsQueue();
		init_info.DescriptorPool = m_GlobalPool->GetDescriptorPool();
		init_info.MinImageCount = 2;
		init_info.ImageCount = 3;
		init_info.CheckVkResultFn = CheckVkResult;
		init_info.MSAASamples = VulkanDevice::GetDevice()->GetMSAASampleCount();

		//ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplVulkan_Init(&init_info, VulkanSwapChain::GetSwapChain()->GetRenderPass());

#define OPENSANS 1
#if OPENSANS
		io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Regular.ttf", 18.0f);
		io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Bold.ttf", 18.0f);
#else
		io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/SourceCodePro/static/SourceCodePro-Regular.ttf", 18.0f);
		io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Bold.ttf", 18.0f);
#endif

		SetDarkThemeColor();

		VkCommandBuffer commandBuffer = VulkanDevice::GetDevice()->BeginSingleTimeCommands();
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
		VulkanDevice::GetDevice()->EndSingleTimeCommands(commandBuffer);
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	void ImGuiLayer::OnDetach()
	{
	}

	void ImGuiLayer::ImGuiBegin()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiLayer::ImGuiRenderandEnd(VkCommandBuffer commandBuffer)
	{
#if IMGUI_VIEWPORTS
		ImGuiIO& io = ImGui::GetIO();
		Application* app = Application::Get();
		io.DisplaySize = ImVec2{ (float)app->GetWindow()->GetWidth(), (float)app->GetWindow()->GetHeight() };
#endif
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

#if IMGUI_VIEWPORTS
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			//GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			//glfwMakeContextCurrent(backup_current_context);
		}
#endif
	}

	void ImGuiLayer::ShutDown()
	{
		ImGui_ImplVulkan_Shutdown();
	}

	void ImGuiLayer::CheckVkResult(VkResult error)
	{
		if (error == 0)
			return;

		VK_CORE_ERROR("[ImGui] Error: VkResult = {0}", error);
	}

	void ImGuiLayer::SetDarkThemeColor() // TODO: Colors are very gray in ImGui, need to Re-Color Everything
	{
		auto& style = ImGui::GetStyle();
		style.WindowBorderSize = 0.0f;

		auto& colors = ImGui::GetStyle().Colors;

		// TODO: Better Names of Color Variables or either remove these variables entirely
		bool squareVal = true;
		glm::vec3 gammaVal = squareVal ? glm::vec3(2.0f) : glm::vec3(1 / 0.44f);
		glm::vec3 colorCode1 = glm::pow(glm::vec3(0.11f, 0.105f, 0.11f), gammaVal);
		glm::vec3 colorCode2 = glm::pow(glm::vec3(0.3f, 0.305f, 0.31f), gammaVal);
		glm::vec3 colorCode3 = glm::pow(glm::vec3(0.15f, 0.1505f, 0.151f), gammaVal);
		glm::vec3 colorCode4 = glm::pow(glm::vec3(0.2f, 0.205f, 0.21f), gammaVal);
		glm::vec3 colorCode5 = glm::pow(glm::vec3(0.38f, 0.3805f, 0.381f), gammaVal);
		glm::vec3 colorCode6 = glm::pow(glm::vec3(0.28f, 0.2805f, 0.281f), gammaVal);
		glm::vec3 colorCode7 = glm::pow(glm::vec3(0.08f, 0.08f, 0.08f), gammaVal);

		colors[ImGuiCol_WindowBg] = ImVec4{ colorCode1.x, colorCode1.y, colorCode1.z, 1.0f };

		colors[ImGuiCol_Header] = ImVec4{ colorCode4.x, colorCode4.y, colorCode4.z, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ colorCode2.x, colorCode2.y, colorCode2.z, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ colorCode3.x, colorCode3.y, colorCode3.z, 1.0f };

		colors[ImGuiCol_Button] = ImVec4{ colorCode4.x, colorCode4.y, colorCode4.z, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ colorCode2.x, colorCode2.y, colorCode2.z, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ colorCode3.x, colorCode3.y, colorCode3.z, 1.0f };

		colors[ImGuiCol_FrameBg] = ImVec4{ colorCode4.x, colorCode4.y, colorCode4.z, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ colorCode2.x, colorCode2.y, colorCode2.z, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ colorCode3.x, colorCode3.y, colorCode3.z, 1.0f };

		colors[ImGuiCol_Tab] = ImVec4{ colorCode3.x, colorCode3.y, colorCode3.z, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ colorCode5.x, colorCode5.y, colorCode5.z, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ colorCode6.x, colorCode6.y, colorCode6.z, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ colorCode3.x, colorCode3.y, colorCode3.z, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ colorCode4.x, colorCode4.y, colorCode4.z, 1.0f };

		colors[ImGuiCol_TitleBg] = ImVec4{ colorCode3.x, colorCode3.y, colorCode3.z, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ colorCode3.x, colorCode3.y, colorCode3.z, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ colorCode3.x, colorCode3.y, colorCode3.z, 1.0f };

		colors[ImGuiCol_MenuBarBg] = ImVec4{ colorCode1.x, colorCode1.y, colorCode1.z, 1.0f };
		colors[ImGuiCol_PopupBg] = ImVec4{ colorCode7.x, colorCode7.y, colorCode7.z, 0.97f };
	}

	void ImGuiLayer::CreateImGuiViewportImages()
	{
		auto device = VulkanDevice::GetDevice();
		auto swapChain = VulkanSwapChain::GetSwapChain();

		VkExtent2D swapChainExtent = swapChain->GetSwapChainExtent();

		m_ViewportImages.resize(swapChain->GetImageCount());
		m_ViewportImageAllocs.resize(swapChain->GetImageCount());

		for (int i = 0; i < m_ViewportImages.size(); i++)
		{
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.format = swapChain->GetSwapChainImageFormat();
			imageInfo.extent.width = swapChainExtent.width;
			imageInfo.extent.height = swapChainExtent.height;
			imageInfo.extent.depth = 1;
			imageInfo.arrayLayers = 1;

			uint32_t mipLevels = static_cast<uint32_t>(std::_Floor_of_log_2(std::max(swapChainExtent.width, swapChainExtent.height))) + 1;
			imageInfo.mipLevels = mipLevels;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			m_ViewportImageAllocs[i] = device->CreateImage(imageInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_ViewportImages[i]);

			VkCommandBuffer cmdBuffer = device->BeginSingleTimeCommands();

			Utils::InsertImageMemoryBarrier(cmdBuffer, m_ViewportImages[i],
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

			device->EndSingleTimeCommands(cmdBuffer);
		}
	}

	void ImGuiLayer::CreateImGuiMultisampleImages()
	{
		auto device = VulkanDevice::GetDevice();
		auto swapChain = VulkanSwapChain::GetSwapChain();

		m_ColorImages.resize(swapChain->GetImageCount());
		m_ColorImageAllocs.resize(swapChain->GetImageCount());
		m_ColorImageViews.resize(swapChain->GetImageCount());

		for (int i = 0; i < m_ColorImages.size(); i++)
		{
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = swapChain->GetSwapChainExtent().width;
			imageInfo.extent.height = swapChain->GetSwapChainExtent().height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = swapChain->GetSwapChainImageFormat();
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			imageInfo.samples = device->GetMSAASampleCount();
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			m_ColorImageAllocs[i] = device->CreateImage(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_ColorImages[i]);

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_ColorImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = swapChain->GetSwapChainImageFormat();
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VK_CHECK_RESULT(vkCreateImageView(device->GetVulkanDevice(), &viewInfo, nullptr, &m_ColorImageViews[i]), "Failed to Create Texture Image View!");
		}
	}

	void ImGuiLayer::CreateImGuiViewportImageViews()
	{
		m_ViewportImageViews.resize(VulkanSwapChain::GetSwapChain()->GetImageCount());

		for (int i = 0; i < m_ViewportImageViews.size(); i++)
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_ViewportImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = VulkanSwapChain::GetSwapChain()->GetSwapChainImageFormat();
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VK_CHECK_RESULT(vkCreateImageView(VulkanDevice::GetDevice()->GetVulkanDevice(), &viewInfo, nullptr, &m_ViewportImageViews[i]), "Failed to create Viewport Image Views!");
		}
	}

	void ImGuiLayer::CreateCommandBuffers()
	{
		auto device = VulkanDevice::GetDevice();

		m_ImGuiCommandBuffers.resize(VulkanSwapChain::GetSwapChain()->GetImageCount());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = device->GetCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(m_ImGuiCommandBuffers.size());

		VK_CHECK_RESULT(vkAllocateCommandBuffers(device->GetVulkanDevice(), &allocInfo, m_ImGuiCommandBuffers.data()), "Failed to Allocate ImGui Command Buffers!");
	}

	void ImGuiLayer::CreateImGuiRenderPass()
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

#if MULTISAMPLING
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
		colorAttachment.samples = device->GetMSAASampleCount();
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
#endif

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = swapChain->FindDepthFormat();
		depthAttachment.samples = device->GetMSAASampleCount();
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentResolveRef = {};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
#if MULTISAMPLING
		subpass.pResolveAttachments = &colorAttachmentResolveRef;
#endif

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

		VK_CHECK_RESULT(vkCreateRenderPass(device->GetVulkanDevice(), &renderPassInfo, nullptr, &m_ViewportRenderPass), "Failed to Create ImGui Viewport Render Pass!");
	}

	void ImGuiLayer::CreateImGuiFramebuffers()
	{
		auto device = VulkanDevice::GetDevice();
		m_Framebuffers.resize(VulkanSwapChain::GetSwapChain()->GetImageCount());

		for (int i = 0; i < m_Framebuffers.size(); i++)
		{
			std::array<VkImageView, 3> attachments = { m_ColorImageViews[i], m_DepthImageViews[i], m_ViewportImageViews[i] };

			VkExtent2D swapChainExtent = VulkanSwapChain::GetSwapChain()->GetSwapChainExtent();
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_ViewportRenderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			VK_CHECK_RESULT(vkCreateFramebuffer(device->GetVulkanDevice(), &framebufferInfo, nullptr, &m_Framebuffers[i]), "Failed to Allocate Viewport Framebuffers!");
		}
	}

	void ImGuiLayer::CreateDepthResources()
	{
		auto swapChain = VulkanSwapChain::GetSwapChain();
		auto device = VulkanDevice::GetDevice();

		VkFormat depthFormat = swapChain->FindDepthFormat();
		VkExtent2D swapChainExtent = swapChain->GetSwapChainExtent();

		m_DepthImages.resize(swapChain->GetImageCount());
		m_DepthImageAllocs.resize(swapChain->GetImageCount());
		m_DepthImageViews.resize(swapChain->GetImageCount());

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
			imageInfo.samples = device->GetMSAASampleCount();
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			m_DepthImageAllocs[i] = device->CreateImage(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImages[i]);

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

}