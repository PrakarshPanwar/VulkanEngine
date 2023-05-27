#include "vulkanpch.h"
#include "VulkanRenderPass.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/VulkanRenderer.h"
#include "VulkanCore/Renderer/Renderer.h"

#include <glm/gtc/type_ptr.hpp>
#include "VulkanContext.h"

namespace VulkanCore {

	namespace Utils {

		static VkFormat VulkanImageFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::RGBA8_SRGB:	   return VK_FORMAT_R8G8B8A8_SRGB;
			case ImageFormat::RGBA8_NORM:	   return VK_FORMAT_R8G8B8A8_SNORM;
			case ImageFormat::RGBA8_UNORM:	   return VK_FORMAT_R8G8B8A8_UNORM;
			case ImageFormat::RGBA16_NORM:	   return VK_FORMAT_R16G16B16A16_SNORM;
			case ImageFormat::RGBA16_UNORM:	   return VK_FORMAT_R16G16B16A16_UNORM;
			case ImageFormat::RGBA16F:		   return VK_FORMAT_R16G16B16A16_SFLOAT;
			case ImageFormat::RGBA32F:		   return VK_FORMAT_R32G32B32A32_SFLOAT;
			case ImageFormat::DEPTH24STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
			case ImageFormat::DEPTH16F:		   return VK_FORMAT_D16_UNORM;
			case ImageFormat::DEPTH32F:		   return VK_FORMAT_D32_SFLOAT;
			default:
				VK_CORE_ASSERT(false, "Format not Supported!");
				return (VkFormat)0;
			}
		}

		static bool IsMultisampled(RenderPassSpecification spec)
		{
			return spec.TargetFramebuffer->GetSpecification().Samples > 1 ? true : false;
		}

		static VkSampleCountFlagBits VulkanSampleCount(uint32_t sampleCount)
		{
			switch (sampleCount)
			{
			case 1:  return VK_SAMPLE_COUNT_1_BIT;
			case 2:  return VK_SAMPLE_COUNT_2_BIT;
			case 4:  return VK_SAMPLE_COUNT_4_BIT;
			case 8:  return VK_SAMPLE_COUNT_8_BIT;
			case 16: return VK_SAMPLE_COUNT_16_BIT;
			case 32: return VK_SAMPLE_COUNT_32_BIT;
			case 64: return VK_SAMPLE_COUNT_64_BIT;
			default:
				VK_CORE_ASSERT(false, "Sample Bit not Supported! Choose Power of 2");
				return VK_SAMPLE_COUNT_1_BIT;
			}
		}
	}

	VulkanRenderPass::VulkanRenderPass(const RenderPassSpecification& spec)
		: m_Specification(spec)
	{
		Renderer::Submit([this]
		{
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
		auto Framebuffer = m_Specification.TargetFramebuffer;

		VkSampleCountFlagBits samples = Utils::VulkanSampleCount(Framebuffer->GetSpecification().Samples);

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
			colorAttachment.finalLayout = Utils::IsMultisampled(m_Specification) ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			attachmentDescriptions.push_back(colorAttachment);
		}

		// Resolve Attachment Description
		VkAttachmentDescription colorAttachmentResolve = {};
		if (Utils::IsMultisampled(m_Specification))
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
				colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachmentDescriptions.push_back(depthAttachment);
		}

		// Color Attachment References
		for (int i = 0; i < Framebuffer->GetColorAttachmentsTextureSpec().size(); ++i)
		{
			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = i;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachmentRefs.push_back(colorAttachmentRef);
		}

		// Resolve Attachment Reference(Only applicable if multisampling is present)
		if (Utils::IsMultisampled(m_Specification))
		{
			for (int i = 0; i < Framebuffer->GetColorAttachmentsTextureSpec().size(); ++i)
			{
				VkAttachmentReference colorAttachmentResolveRef = {};
				colorAttachmentResolveRef.attachment = static_cast<uint32_t>(attachmentRefs.size());
				colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				attachmentRefs.push_back(colorAttachmentResolveRef);
			}
		}

		// Depth Attachment Reference
		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = static_cast<uint32_t>(attachmentRefs.size());
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachmentRefs.push_back(depthAttachmentRef);

		VkSubpassDescription subpass = {}; // TODO: Changes need to be made
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(Framebuffer->GetColorAttachmentsTextureSpec().size());
		subpass.pColorAttachments = attachmentRefs.data();

		// TODO: We are using single resolve attachment but,
		// I think we need a vector of it for multiple color attachments
		subpass.pDepthStencilAttachment = Framebuffer->HasDepthAttachment() ? &depthAttachmentRef : nullptr;
		subpass.pResolveAttachments = Utils::IsMultisampled(m_Specification) ?
			attachmentRefs.data() + Framebuffer->GetColorAttachmentsTextureSpec().size() : nullptr;

		VkSubpassDependency dependency = {}; // TODO: Changes need to be made
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstSubpass = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
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

		m_ClearValues.resize(attachmentDescriptions.size());
		m_AttachmentDescriptions = attachmentDescriptions;
		Framebuffer->CreateFramebuffer(m_RenderPass);
	}

	void VulkanRenderPass::RecreateFramebuffers(uint32_t width, uint32_t height)
	{
		auto Framebuffer = m_Specification.TargetFramebuffer;
		Framebuffer->Resize(width, height);
		Framebuffer->CreateFramebuffer(m_RenderPass);
	}

	void VulkanRenderPass::Begin(VkCommandBuffer beginCmd)
	{
		auto Framebuffer = m_Specification.TargetFramebuffer;
		const FramebufferSpecification fbSpec = Framebuffer->GetSpecification();
		const VkExtent2D framebufferExtent = { fbSpec.Width, fbSpec.Height };

		Renderer::Submit([this, beginCmd, Framebuffer, framebufferExtent, fbSpec]
		{
			VK_CORE_PROFILE_FN("VulkanRenderPass::Begin");

			VkRenderPassBeginInfo beginPassInfo{};
			beginPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			beginPassInfo.renderPass = m_RenderPass;
			beginPassInfo.framebuffer = Framebuffer->GetVulkanFramebuffers()[Renderer::RT_GetCurrentFrameIndex()];
			beginPassInfo.renderArea.offset = { 0, 0 };
			beginPassInfo.renderArea.extent = framebufferExtent;

			// TODO: We may change this in future as there will be multiple allocation/deallocation in
			// clearValues vector
			std::vector<VkClearValue> clearValues{ m_AttachmentDescriptions.size() };
			for (uint32_t i = 0; i < Framebuffer->GetColorAttachmentsTextureSpec().size(); ++i)
				clearValues[i].color = { fbSpec.ClearColor.x, fbSpec.ClearColor.y, fbSpec.ClearColor.z, fbSpec.ClearColor.w };

			clearValues[clearValues.size() - 1].depthStencil = { 1.0f, 0 };

			beginPassInfo.clearValueCount = (uint32_t)clearValues.size();
			beginPassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(beginCmd, &beginPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport{};
			viewport.x = 0.0f;
			viewport.y = static_cast<float>(fbSpec.Height);
			viewport.width = static_cast<float>(fbSpec.Width);
			viewport.height = -static_cast<float>(fbSpec.Height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkRect2D scissor{ { 0, 0 }, framebufferExtent };
			vkCmdSetViewport(beginCmd, 0, 1, &viewport);
			vkCmdSetScissor(beginCmd, 0, 1, &scissor);
		});
	}

	void VulkanRenderPass::Begin(std::shared_ptr<VulkanRenderCommandBuffer> beginCmd)
	{
		auto Framebuffer = m_Specification.TargetFramebuffer;

		Renderer::Submit([this, beginCmd, Framebuffer]
		{
			VK_CORE_PROFILE_FN("VulkanRenderPass::Begin");

			const FramebufferSpecification fbSpec = Framebuffer->GetSpecification();
			const VkExtent2D framebufferExtent = { fbSpec.Width, fbSpec.Height };

			VkCommandBuffer vulkanCommandBuffer = beginCmd->RT_GetActiveCommandBuffer();

			VkRenderPassBeginInfo beginPassInfo{};
			beginPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			beginPassInfo.renderPass = m_RenderPass;
			beginPassInfo.framebuffer = Framebuffer->GetVulkanFramebuffers()[Renderer::RT_GetCurrentFrameIndex()];
			beginPassInfo.renderArea.offset = { 0, 0 };
			beginPassInfo.renderArea.extent = framebufferExtent;

			for (uint32_t i = 0; i < Framebuffer->GetColorAttachmentsTextureSpec().size(); ++i)
				m_ClearValues[i].color = { fbSpec.ClearColor.x, fbSpec.ClearColor.y, fbSpec.ClearColor.z, fbSpec.ClearColor.w };

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
			vkCmdSetViewport(vulkanCommandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(vulkanCommandBuffer, 0, 1, &scissor);
		});
	}

	void VulkanRenderPass::End(VkCommandBuffer endCmd)
	{
		Renderer::Submit([endCmd] { vkCmdEndRenderPass(endCmd); });
	}

	void VulkanRenderPass::End(std::shared_ptr<VulkanRenderCommandBuffer> endCmd)
	{
		Renderer::Submit([endCmd]
		{
			VkCommandBuffer vulkanCmdBuffer = endCmd->RT_GetActiveCommandBuffer();
			vkCmdEndRenderPass(vulkanCmdBuffer);
		});
	}

}