#include "vulkanpch.h"
#include "VulkanFramebuffer.h"

#include "VulkanSwapChain.h"
#include "VulkanCore/Core/Assert.h";
#include "VulkanCore/Core/Log.h"

namespace VulkanCore {

	namespace Utils {

		static bool IsDepthFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::DEPTH24STENCIL8: return true;
			case ImageFormat::DEPTH16F:		return true;
			case ImageFormat::DEPTH32F:		return true;
			default:
				return false;
			}
		}

	}

	VulkanFramebuffer::VulkanFramebuffer(const FramebufferSpecification& spec)
		: m_Specification(spec)
	{
		for (auto specification : m_Specification.Attachments.Attachments)
		{
			if (Utils::IsDepthFormat(specification.TextureFormat))
				m_DepthAttachmentSpecification = specification;
			else
				m_ColorAttachmentSpecifications.emplace_back(specification);
		}

		m_ColorAttachments.resize(m_ColorAttachmentSpecifications.size());
		Invalidate();
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{

	}

	void VulkanFramebuffer::Invalidate()
	{
		for (auto& attachment : m_ColorAttachmentSpecifications)
		{
			std::vector<VulkanImage> AttachmentImages;

			// Adding 3 Images in Flight
			for (int i = 0; i < VulkanSwapChain::MaxFramesInFlight; ++i)
			{
				ImageSpecification spec;
				spec.Width = m_Specification.Width;
				spec.Height = m_Specification.Height;
				spec.Samples = m_Specification.Samples;
				spec.Format = attachment.TextureFormat;
				spec.Usage = ImageUsage::Attachment;

				auto& attachmentColorImage = AttachmentImages.emplace_back(spec);
				attachmentColorImage.Invalidate();
			}

			m_ColorAttachments.emplace_back(AttachmentImages);
		}

		for (int i = 0; i < VulkanSwapChain::MaxFramesInFlight; i++)
		{
			ImageSpecification spec;
			spec.Width = m_Specification.Width;
			spec.Height = m_Specification.Height;
			spec.Samples = m_Specification.Samples;
			spec.Format = m_DepthAttachmentSpecification.TextureFormat;
			spec.Usage = ImageUsage::Attachment;

			auto& depthImage = m_DepthAttachment.emplace_back(spec);
			depthImage.Invalidate();
		}
	}

	void VulkanFramebuffer::CreateFramebuffer()
	{
	}

	void VulkanFramebuffer::Release()
	{
		for (auto framebuffer : m_Framebuffers)
			vkDestroyFramebuffer(VulkanDevice::GetDevice()->GetVulkanDevice(), framebuffer, nullptr);
	}

}