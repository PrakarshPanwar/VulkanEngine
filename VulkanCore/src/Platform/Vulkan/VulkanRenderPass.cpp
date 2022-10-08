#include "vulkanpch.h"
#include "VulkanRenderPass.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

namespace VulkanCore {

	namespace Utils {

		static VkFormat VulkanImageFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::RGBA:			   return VK_FORMAT_R8G8B8A8_SRGB;
			case ImageFormat::RGBA8:		   return VK_FORMAT_R8G8B8A8_SNORM;
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

		static bool IsMultiSampled(RenderPassSpecification spec)
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

	}

	void VulkanRenderPass::Invalidate()
	{
		auto device = VulkanDevice::GetDevice();

		auto Framebuffer = m_Specification.TargetFramebuffer;
		VkSampleCountFlagBits samples = Utils::VulkanSampleCount(Framebuffer->GetSpecification().Samples);

		VkAttachmentDescription colorAttachmentResolve = {};
		if (Utils::IsMultiSampled(m_Specification))
		{
			colorAttachmentResolve.format = Utils::VulkanImageFormat(ImageFormat::RGBA8);
			colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			colorAttachmentResolve.flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
		}

		for (auto attachmentSpec : Framebuffer->GetColorAttachments())
		{
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = Utils::VulkanImageFormat(attachmentSpec.TextureFormat);
			colorAttachment.samples = samples;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			m_AttachmentDescriptions.push_back(colorAttachment);
		}

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = Utils::VulkanImageFormat(Framebuffer->GetDepthAttachment().TextureFormat);
		depthAttachment.samples = samples;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		for (int i = 0; i < m_AttachmentDescriptions.size(); i++)
		{
			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = i;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			m_AttachmentReferences.push_back(colorAttachmentRef);
		}

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = static_cast<uint32_t>(m_AttachmentDescriptions.size());
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		m_AttachmentReferences.push_back(depthAttachmentRef);

		if (Utils::IsMultiSampled(m_Specification))
		{
			VkAttachmentReference colorAttachmentResolveRef = {};
			colorAttachmentResolveRef.attachment = static_cast<uint32_t>(m_AttachmentDescriptions.size() + 1);
			colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			m_AttachmentReferences.push_back(colorAttachmentResolveRef);
		}

		VkSubpassDescription subpass = {}; // TODO: Changes need to be made
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(Framebuffer->GetColorAttachments().size());
		subpass.pColorAttachments = m_AttachmentReferences.data();
		subpass.pDepthStencilAttachment = &m_AttachmentReferences[m_AttachmentReferences.size() - 2];
		subpass.pResolveAttachments = Utils::IsMultiSampled(m_Specification) ?
			&m_AttachmentReferences[m_AttachmentReferences.size() - 1] : nullptr;

		VkSubpassDependency dependency = {}; // TODO: Changes need to be made
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstSubpass = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(m_AttachmentDescriptions.size());
		renderPassInfo.pAttachments = m_AttachmentDescriptions.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VK_CHECK_RESULT(vkCreateRenderPass(device->GetVulkanDevice(), &renderPassInfo, nullptr, &m_RenderPass), "Failed to Create Scene Render Pass!");
	}

}