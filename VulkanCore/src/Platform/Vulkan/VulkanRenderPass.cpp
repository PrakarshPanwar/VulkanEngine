#include "vulkanpch.h"
#include "VulkanRenderPass.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "Utils/ImageUtils.h"

#include <glm/gtc/type_ptr.hpp>
#include "VulkanContext.h"

namespace VulkanCore {

	VulkanRenderPass::VulkanRenderPass(const RenderPassSpecification& spec)
		: m_Specification(spec)
	{
		Renderer::Submit([this]
		{
			if (m_Specification.TargetFramebuffer->GetSpecification().ReadDepthTexture)
				InvalidateWithDepthTexture();
			else
				Invalidate();
		});
	}

	VulkanRenderPass::~VulkanRenderPass()
	{
		auto device = VulkanContext::GetCurrentDevice();

		if (m_RenderPass == nullptr)
			return;

		vkDestroyRenderPass(device->GetVulkanDevice(), m_RenderPass, nullptr);
	}

	void VulkanRenderPass::Invalidate()
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto Framebuffer = std::static_pointer_cast<VulkanFramebuffer>(m_Specification.TargetFramebuffer);

		VkSampleCountFlagBits samples = Utils::VulkanSampleCount(Framebuffer->GetSpecification().Samples);

		bool multisampled = Utils::IsMultisampled(m_Specification);
		std::vector<VkAttachmentDescription> attachmentDescriptions;
		std::vector<VkAttachmentReference> attachmentRefs;

		// Color Attachments Description
		for (const auto& attachmentSpec : Framebuffer->GetColorAttachmentsTextureSpec())
		{
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = Utils::VulkanImageFormat(attachmentSpec.ImgFormat);
			colorAttachment.samples = samples;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = multisampled ? VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;

			attachmentDescriptions.push_back(colorAttachment);
		}

		// Resolve Attachment Description
		VkAttachmentDescription colorAttachmentResolve = {};
		if (multisampled)
		{
			for (const auto& attachmentSpec : Framebuffer->GetColorAttachmentsTextureSpec())
			{
				colorAttachmentResolve.format = Utils::VulkanImageFormat(attachmentSpec.ImgFormat);
				colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
				colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
				colorAttachmentResolve.flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
				attachmentDescriptions.push_back(colorAttachmentResolve);
			}
		}

		// Depth Attachment Description
		if (Framebuffer->HasDepthAttachment())
		{
			VkAttachmentDescription depthAttachment = {};
			depthAttachment.format = Utils::VulkanImageFormat(Framebuffer->GetDepthAttachmentTextureSpec().ImgFormat);
			depthAttachment.samples = samples;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
			attachmentDescriptions.push_back(depthAttachment);
		}

		// Color Attachment References
		for (int i = 0; i < Framebuffer->GetColorAttachmentsTextureSpec().size(); ++i)
		{
			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = i;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
			attachmentRefs.push_back(colorAttachmentRef);
		}

		// Resolve Attachment Reference(Only applicable if multisampling is present)
		if (multisampled)
		{
			for (int i = 0; i < Framebuffer->GetColorAttachmentsTextureSpec().size(); ++i)
			{
				VkAttachmentReference colorAttachmentResolveRef = {};
				colorAttachmentResolveRef.attachment = static_cast<uint32_t>(attachmentRefs.size());
				colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
				attachmentRefs.push_back(colorAttachmentResolveRef);
			}
		}

		// Depth Attachment Reference
		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = static_cast<uint32_t>(attachmentRefs.size());
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		attachmentRefs.push_back(depthAttachmentRef);

		VkSubpassDescription subpass = {}; // TODO: Changes need to be made
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(Framebuffer->GetColorAttachmentsTextureSpec().size());
		subpass.pColorAttachments = attachmentRefs.data();
		subpass.pDepthStencilAttachment = Framebuffer->HasDepthAttachment() ? &depthAttachmentRef : nullptr;
		subpass.pResolveAttachments = multisampled ? attachmentRefs.data() + Framebuffer->GetColorAttachmentsTextureSpec().size() : nullptr;

		VkSubpassDependency dependency = {}; // TODO: Changes need to be made
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

		// TODO: There could be multiple subpasses/framebuffers, needs to be changed in future
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
		renderPassInfo.pAttachments = attachmentDescriptions.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VK_CHECK_RESULT(vkCreateRenderPass(device->GetVulkanDevice(), &renderPassInfo, nullptr, &m_RenderPass), "Failed to Create Scene Render Pass!");

		m_AttachmentDescriptions = attachmentDescriptions;
		m_ClearValues.resize(m_AttachmentDescriptions.size());
		Framebuffer->CreateFramebuffer(m_RenderPass);
	}

	void VulkanRenderPass::InvalidateWithDepthTexture()
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto Framebuffer = std::static_pointer_cast<VulkanFramebuffer>(m_Specification.TargetFramebuffer);

		VkSampleCountFlagBits samples = Utils::VulkanSampleCount(Framebuffer->GetSpecification().Samples);

		bool multisampled = Utils::IsMultisampled(m_Specification);
		std::vector<VkAttachmentDescription2> attachmentDescriptions;
		std::vector<VkAttachmentReference2> attachmentRefs;
		std::vector<VkAttachmentReference2> attachmentResolveRefs;

		// Color Attachments Description
		for (const auto& attachmentSpec : Framebuffer->GetColorAttachmentsTextureSpec())
		{
			VkAttachmentDescription2 colorAttachment = {};
			colorAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
			colorAttachment.format = Utils::VulkanImageFormat(attachmentSpec.ImgFormat);
			colorAttachment.samples = samples;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = multisampled ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = multisampled ? VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL :
				VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;

			attachmentDescriptions.push_back(colorAttachment);
		}

		// Resolve Attachment Description
		VkAttachmentDescription2 colorAttachmentResolve = {};
		if (multisampled)
		{
			for (const auto& attachmentSpec : Framebuffer->GetColorAttachmentsTextureSpec())
			{
				colorAttachmentResolve.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
				colorAttachmentResolve.format = Utils::VulkanImageFormat(attachmentSpec.ImgFormat);
				colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
				colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
				colorAttachmentResolve.flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
				attachmentDescriptions.push_back(colorAttachmentResolve);
			}
		}

		// Depth Attachment Description
		if (Framebuffer->HasDepthAttachment())
		{
			VkAttachmentDescription2 depthAttachment = {};
			depthAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
			depthAttachment.format = Utils::VulkanImageFormat(Framebuffer->GetDepthAttachmentTextureSpec().ImgFormat);
			depthAttachment.samples = samples;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = multisampled ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = multisampled ? VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
			attachmentDescriptions.push_back(depthAttachment);

			if (Framebuffer->GetSpecification().ReadDepthTexture && multisampled)
			{
				VkAttachmentDescription2 depthAttachmentResolve = {};
				depthAttachmentResolve.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
				depthAttachmentResolve.format = Utils::VulkanImageFormat(Framebuffer->GetDepthAttachmentTextureSpec().ImgFormat);
				depthAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
				depthAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				depthAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				depthAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				depthAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				depthAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				depthAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
				depthAttachmentResolve.flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
				attachmentDescriptions.push_back(depthAttachmentResolve);
			}
		}

		// Color Attachment References
		for (int i = 0; i < Framebuffer->GetColorAttachmentsTextureSpec().size(); ++i)
		{
			VkAttachmentReference2 colorAttachmentRef = {};
			colorAttachmentRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
			colorAttachmentRef.attachment = i;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
			attachmentRefs.push_back(colorAttachmentRef);
		}

		// Resolve Attachment Reference(Only applicable if multisampling is present)
		if (multisampled)
		{
			for (int i = 0; i < Framebuffer->GetColorAttachmentsTextureSpec().size(); ++i)
			{
				VkAttachmentReference2 colorAttachmentResolveRef = {};
				colorAttachmentResolveRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
				colorAttachmentResolveRef.attachment = static_cast<uint32_t>(attachmentRefs.size());
				colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
				attachmentRefs.push_back(colorAttachmentResolveRef);
				attachmentResolveRefs.push_back(colorAttachmentResolveRef);
			}
		}

		// Depth Attachment Reference
		VkAttachmentReference2 depthAttachmentRef = {};
		depthAttachmentRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
		depthAttachmentRef.attachment = static_cast<uint32_t>(attachmentRefs.size());
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		attachmentRefs.push_back(depthAttachmentRef);

		VkAttachmentReference2 depthAttachmentResolveRef = {};
		depthAttachmentResolveRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
		depthAttachmentResolveRef.attachment = static_cast<uint32_t>(attachmentRefs.size());
		depthAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		attachmentRefs.push_back(depthAttachmentResolveRef);

		// Depth Resolve Extension
		VkSubpassDescriptionDepthStencilResolve depthResolveExt = {};
		depthResolveExt.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
		depthResolveExt.depthResolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
		depthResolveExt.stencilResolveMode = VK_RESOLVE_MODE_NONE;
		depthResolveExt.pDepthStencilResolveAttachment = &depthAttachmentResolveRef;

		VkSubpassDescription2 subpass = {}; // TODO: Changes need to be made
		subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(Framebuffer->GetColorAttachmentsTextureSpec().size());
		subpass.pColorAttachments = attachmentRefs.data();
		subpass.pDepthStencilAttachment = Framebuffer->HasDepthAttachment() ? &depthAttachmentRef : nullptr;
		subpass.pResolveAttachments = multisampled ? attachmentResolveRefs.data() : nullptr;
		subpass.pNext = multisampled ? &depthResolveExt : nullptr;

		VkMemoryBarrier2 subpassBarrier = {};
		subpassBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
		subpassBarrier.srcAccessMask = 0;
		subpassBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		subpassBarrier.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpassBarrier.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

		VkSubpassDependency2 dependency = {}; // TODO: Changes need to be made
		dependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.pNext = &subpassBarrier;

		// TODO: There could be multiple subpasses/framebuffers, needs to be changed in future
		VkRenderPassCreateInfo2 renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
		renderPassInfo.pAttachments = attachmentDescriptions.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VK_CHECK_RESULT(vkCreateRenderPass2(device->GetVulkanDevice(), &renderPassInfo, nullptr, &m_RenderPass), "Failed to Create Scene Render Pass!");

		m_AttachmentDescriptions2 = attachmentDescriptions;
		m_ClearValues.resize(m_AttachmentDescriptions2.size());
		Framebuffer->CreateFramebuffer(m_RenderPass);
	}

	void VulkanRenderPass::RecreateFramebuffers(uint32_t width, uint32_t height)
	{
		auto Framebuffer = std::static_pointer_cast<VulkanFramebuffer>(m_Specification.TargetFramebuffer);
		Framebuffer->Resize(width, height);
		Framebuffer->CreateFramebuffer(m_RenderPass);
	}

	void VulkanRenderPass::Begin(const std::shared_ptr<VulkanRenderCommandBuffer>& beginCmd)
	{
		Renderer::Submit([this, beginCmd, Framebuffer = std::static_pointer_cast<VulkanFramebuffer>(m_Specification.TargetFramebuffer)]
		{
			VK_CORE_PROFILE_FN("VulkanRenderPass::Begin");

			const FramebufferSpecification fbSpec = Framebuffer->GetSpecification();
			const VkExtent2D framebufferExtent = { fbSpec.Width, fbSpec.Height };
			bool multisampled = Utils::IsMultisampled(m_Specification);

			VkCommandBuffer vulkanCommandBuffer = beginCmd->RT_GetActiveCommandBuffer();

			VkRenderPassBeginInfo beginPassInfo{};
			beginPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			beginPassInfo.renderPass = m_RenderPass;
			beginPassInfo.framebuffer = Framebuffer->GetVulkanFramebuffers()[Renderer::RT_GetCurrentFrameIndex()];
			beginPassInfo.renderArea.offset = { 0, 0 };
			beginPassInfo.renderArea.extent = framebufferExtent;

			for (uint32_t i = 0; i < Framebuffer->GetColorAttachmentsTextureSpec().size(); ++i)
				m_ClearValues[i].color = { fbSpec.ClearColor.x, fbSpec.ClearColor.y, fbSpec.ClearColor.z, fbSpec.ClearColor.w };

			if (m_Specification.TargetFramebuffer->GetSpecification().ReadDepthTexture && multisampled)
			{
				m_ClearValues[m_ClearValues.size() - 2].depthStencil = { 1.0f, 0 };
				m_ClearValues[m_ClearValues.size() - 1].depthStencil = { 1.0f, 0 };
			}
			else
				m_ClearValues[m_ClearValues.size() - 1].depthStencil = { 1.0f, 0 };

			beginPassInfo.clearValueCount = (uint32_t)m_ClearValues.size();
			beginPassInfo.pClearValues = m_ClearValues.data();

			vkCmdBeginRenderPass(vulkanCommandBuffer, &beginPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport{};
			viewport.x = 0.0f;
			viewport.y = static_cast<float>(fbSpec.Height);
			viewport.width = static_cast<float>(fbSpec.Width);
			viewport.height = -static_cast<float>(fbSpec.Height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkRect2D scissor{ { 0, 0 }, framebufferExtent };
			vkCmdSetViewportWithCount(vulkanCommandBuffer, 1, &viewport);
			vkCmdSetScissorWithCount(vulkanCommandBuffer, 1, &scissor);
		});
	}

	void VulkanRenderPass::End(const std::shared_ptr<VulkanRenderCommandBuffer>& endCmd)
	{
		Renderer::Submit([endCmd]
		{
			VkCommandBuffer vulkanCmdBuffer = endCmd->RT_GetActiveCommandBuffer();
			vkCmdEndRenderPass(vulkanCmdBuffer);
		});
	}

}
