#include "vulkanpch.h"
#include "VulkanRenderPass.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "Utils/ImageUtils.h"

#include <glm/gtc/type_ptr.hpp>
#include "VulkanContext.h"

namespace VulkanCore {

	namespace Utils {

		static VkAttachmentLoadOp VulkanAttachmentLoadOp(AttachmentLoadOp loadOp)
		{
			switch (loadOp)
			{
				case AttachmentLoadOp::DontCare: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				case AttachmentLoadOp::Clear:	 return VK_ATTACHMENT_LOAD_OP_CLEAR;
				case AttachmentLoadOp::Load:	 return VK_ATTACHMENT_LOAD_OP_LOAD;
				default:
					VK_CORE_ASSERT(false, "Attachment Load Operation not defined!");
					return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			}
		}

		static VkAttachmentStoreOp VulkanAttachmentStoreOp(AttachmentStoreOp storeOp)
		{
			switch (storeOp)
			{
				case AttachmentStoreOp::DontCare: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
				case AttachmentStoreOp::Store:    return VK_ATTACHMENT_STORE_OP_STORE;
				default:
					VK_CORE_ASSERT(false, "Attachment Store Operation not defined!");
					return VK_ATTACHMENT_STORE_OP_DONT_CARE;
			}
		}

		static VkImageLayout VulkanAttachmentInitialLayout(AttachmentLoadOp loadOp)
		{
			switch (loadOp)
			{
				case AttachmentLoadOp::DontCare: return VK_IMAGE_LAYOUT_UNDEFINED;
				case AttachmentLoadOp::Load:     return VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
				case AttachmentLoadOp::Clear:    return VK_IMAGE_LAYOUT_UNDEFINED;
				default:
					VK_CORE_ASSERT(false, "Invalid Attachment Load Operation for layout determination!");
					return VK_IMAGE_LAYOUT_UNDEFINED;
			}
		}

	}

	VulkanRenderPass::VulkanRenderPass(const RenderPassSpecification& spec)
		: m_Specification(spec)
	{
		const auto& FramebufferColorAttachments = m_Specification.TargetFramebuffer->GetColorAttachmentsTextureSpec();
		if ((m_Specification.ColorLoadOps.size() != FramebufferColorAttachments.size()) ||
			(m_Specification.ColorStoreOps.size()) != FramebufferColorAttachments.size())
		{
			bool multisampled = Utils::IsMultisampled(m_Specification);

			m_Specification.ColorLoadOps.resize(FramebufferColorAttachments.size(), AttachmentLoadOp::Clear);
			m_Specification.ColorStoreOps.resize(FramebufferColorAttachments.size(), multisampled ? AttachmentStoreOp::DontCare : AttachmentStoreOp::Store);
		}

		Renderer::Submit([this]
		{
			Invalidate();
		});
	}

	VulkanRenderPass::~VulkanRenderPass()
	{
		if (m_RenderPass)
			Release();
	}

	void VulkanRenderPass::Invalidate()
	{
		if (m_RenderPass)
			Release();

		auto device = VulkanContext::GetCurrentDevice();
		auto Framebuffer = std::static_pointer_cast<VulkanFramebuffer>(m_Specification.TargetFramebuffer);
		const auto& FramebufferColorAttachments = Framebuffer->GetColorAttachmentsTextureSpec();

		VkSampleCountFlagBits samples = Utils::VulkanSampleCount(Framebuffer->GetSpecification().Samples);

		bool multisampled = Utils::IsMultisampled(m_Specification);
		std::vector<VkAttachmentDescription2> attachmentDescriptions;
		std::vector<VkAttachmentReference2> attachmentRefs;

		// Color Attachments Description
		for (uint32_t i = 0; const auto& attachmentSpec : FramebufferColorAttachments)
		{
			VkAttachmentDescription2 colorAttachment = {};
			colorAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
			colorAttachment.format = Utils::VulkanImageFormat(attachmentSpec.ImgFormat);
			colorAttachment.samples = samples;
			colorAttachment.loadOp = Utils::VulkanAttachmentLoadOp(m_Specification.ColorLoadOps[i]);
			colorAttachment.storeOp = Utils::VulkanAttachmentStoreOp(m_Specification.ColorStoreOps[i]);
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.initialLayout = Utils::VulkanAttachmentInitialLayout(m_Specification.ColorLoadOps[i]);
			colorAttachment.finalLayout = multisampled ? VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;

			attachmentDescriptions.push_back(colorAttachment);
			++i;
		}

		// Resolve Attachment Description
		VkAttachmentDescription2 colorAttachmentResolve = {};
		if (multisampled)
		{
			for (const auto& attachmentSpec : FramebufferColorAttachments)
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
			depthAttachment.loadOp = Utils::VulkanAttachmentLoadOp(m_Specification.DepthLoadOp);
			depthAttachment.storeOp = Utils::VulkanAttachmentStoreOp(m_Specification.DepthStoreOp);
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = Utils::VulkanAttachmentInitialLayout(m_Specification.DepthLoadOp);
			depthAttachment.finalLayout = multisampled ? VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
			attachmentDescriptions.push_back(depthAttachment);

			if (multisampled && Framebuffer->GetSpecification().ReadDepthTexture)
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
		for (int i = 0; i < FramebufferColorAttachments.size(); ++i)
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
			for (int i = 0; i < FramebufferColorAttachments.size(); ++i)
			{
				VkAttachmentReference2 colorAttachmentResolveRef = {};
				colorAttachmentResolveRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
				colorAttachmentResolveRef.attachment = static_cast<uint32_t>(attachmentRefs.size());
				colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
				attachmentRefs.push_back(colorAttachmentResolveRef);
			}
		}

		// Depth Attachment Reference
		VkAttachmentReference2 depthAttachmentRef = {};
		depthAttachmentRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
		depthAttachmentRef.attachment = static_cast<uint32_t>(attachmentRefs.size());
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;

		VkAttachmentReference2 depthAttachmentResolveRef = {};
		depthAttachmentResolveRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
		depthAttachmentResolveRef.attachment = static_cast<uint32_t>(attachmentRefs.size()) + 1;
		depthAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;

		// Depth Resolve Extension
		VkSubpassDescriptionDepthStencilResolve depthResolveExt = {};
		depthResolveExt.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
		depthResolveExt.depthResolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
		depthResolveExt.stencilResolveMode = VK_RESOLVE_MODE_NONE;
		depthResolveExt.pDepthStencilResolveAttachment = &depthAttachmentResolveRef;

		VkSubpassDescription2 subpass = {}; // TODO: Changes need to be made
		subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(FramebufferColorAttachments.size());
		subpass.pColorAttachments = attachmentRefs.data();
		subpass.pDepthStencilAttachment = Framebuffer->HasDepthAttachment() ? &depthAttachmentRef : nullptr;
		subpass.pResolveAttachments = multisampled ? attachmentRefs.data() + subpass.colorAttachmentCount : nullptr;
		subpass.pNext = multisampled && Framebuffer->GetSpecification().ReadDepthTexture ? &depthResolveExt : nullptr;

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

		m_AttachmentDescriptions = attachmentDescriptions;
		m_ClearValues.resize(m_AttachmentDescriptions.size());
		Framebuffer->CreateFramebuffer(m_RenderPass);
	}

	void VulkanRenderPass::Release()
	{
		auto device = VulkanContext::GetCurrentDevice();

		if (m_RenderPass == nullptr)
			return;

		vkDestroyRenderPass(device->GetVulkanDevice(), m_RenderPass, nullptr);
	}

	void VulkanRenderPass::RecreateFramebuffers(uint32_t width, uint32_t height)
	{
		std::bitset<11> resizeBitFlag = 0;
		bool multisampled = Utils::IsMultisampled(m_Specification);

		for (uint32_t i = 0; i < m_Specification.ColorLoadOps.size(); ++i)
		{
			if (m_Specification.ColorLoadOps[i] != AttachmentLoadOp::Load)
				resizeBitFlag.set(i);

			if (multisampled)
				resizeBitFlag.set(i + m_Specification.ColorLoadOps.size());
		}

		resizeBitFlag.set(10, m_Specification.DepthLoadOp != AttachmentLoadOp::Load);

		auto Framebuffer = std::static_pointer_cast<VulkanFramebuffer>(m_Specification.TargetFramebuffer);
		Framebuffer->Resize(width, height, resizeBitFlag);
		Framebuffer->CreateFramebuffer(m_RenderPass);
	}

	void VulkanRenderPass::SetColorAttachment(uint32_t index, const std::vector<std::shared_ptr<Image2D>>& colorImages)
	{
		VK_CORE_ASSERT(m_Specification.ColorLoadOps[index] == AttachmentLoadOp::Load, "Invalid Load Operation is used!");

		auto Framebuffer = std::static_pointer_cast<VulkanFramebuffer>(m_Specification.TargetFramebuffer);
		Framebuffer->SetColorAttachments(index, colorImages);
		Framebuffer->CreateFramebuffer(m_RenderPass);
	}

	void VulkanRenderPass::SetDepthAttachment(const std::vector<std::shared_ptr<Image2D>>& depthImages)
	{
		VK_CORE_ASSERT(m_Specification.DepthLoadOp == AttachmentLoadOp::Load, "Invalid Load Operation is used!");

		auto Framebuffer = std::static_pointer_cast<VulkanFramebuffer>(m_Specification.TargetFramebuffer);
		Framebuffer->SetDepthAttachments(depthImages);
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

			for (uint32_t i = 0; auto& FramebufferColorAttachment : Framebuffer->GetColorAttachmentsTextureSpec())
			{
				float ClearColor = FramebufferColorAttachment.ClearColor;
				m_ClearValues[i].color = { ClearColor, ClearColor, ClearColor, ClearColor };

				++i;
			}

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
