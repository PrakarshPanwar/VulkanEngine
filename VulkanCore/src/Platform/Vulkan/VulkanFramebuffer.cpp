#include "vulkanpch.h"
#include "VulkanFramebuffer.h"

#include "VulkanSwapChain.h"
#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

namespace VulkanCore {

	namespace Utils {

		static bool IsDepthFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::DEPTH24STENCIL8: return true;
			case ImageFormat::DEPTH16F:		   return true;
			case ImageFormat::DEPTH32F:		   return true;
			default:
				return false;
			}
		}

		static bool IsMultisampled(FramebufferSpecification spec)
		{
			return spec.Samples > 1 ? true : false;
		}

	}

	uint32_t VulkanFramebuffer::s_InstanceCount = 0;

	VulkanFramebuffer::VulkanFramebuffer(const FramebufferSpecification& spec)
		: m_Specification(spec)
	{
		for (auto specification : m_Specification.Attachments.Attachments)
		{
			if (Utils::IsDepthFormat(specification.ImageFormat))
				m_DepthAttachmentSpecification = specification;
			else
				m_ColorAttachmentSpecifications.emplace_back(specification);
		}

		s_InstanceCount++;
		Invalidate();
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{
		if (m_Framebuffers.at(0) == nullptr)
			return;

		VK_CORE_TRACE("Framebuffers Instances: {}", s_InstanceCount);
		Release();
	}

	const std::vector<VulkanImage>& VulkanFramebuffer::GetResolveAttachment() const
	{
		for (auto& images : m_ColorAttachments)
		{
			if (images[0].GetSpecification().Samples == 1)
				return images;
		}

		VK_CORE_ASSERT(false, "No Resolve Images present");
		return {};
	}

	void VulkanFramebuffer::Invalidate()
	{
		auto device = VulkanContext::GetCurrentDevice();

		uint32_t attachmentSize = static_cast<uint32_t>(m_Specification.Samples > 1 ? (m_ColorAttachmentSpecifications.size() + 1) : m_ColorAttachmentSpecifications.size());
		m_ColorAttachments.reserve(attachmentSize);

		// Image Creation for Color Attachments
		for (auto& attachment : m_ColorAttachmentSpecifications)
		{
			std::vector<VulkanImage> AttachmentImages;
			AttachmentImages.reserve(VulkanSwapChain::MaxFramesInFlight);

			// Adding 3 Images in Flight(Only for this system)
			for (int i = 0; i < VulkanSwapChain::MaxFramesInFlight; ++i)
			{
				ImageSpecification spec;
				spec.Width = m_Specification.Width;
				spec.Height = m_Specification.Height;
				spec.Samples = m_Specification.Samples;
				spec.Format = attachment.ImageFormat;
				spec.Usage = ImageUsage::Attachment;

				auto& attachmentColorImage = AttachmentImages.emplace_back(spec);
				attachmentColorImage.Invalidate();

				if (!Utils::IsMultisampled(m_Specification))
				{
					VkCommandBuffer barrierCmd = device->GetCommandBuffer();

					Utils::InsertImageMemoryBarrier(barrierCmd, 
						attachmentColorImage.GetVulkanImageInfo().Image,
						VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
						VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
						VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

					device->FlushCommandBuffer(barrierCmd);
				}
			}

			m_ColorAttachments.emplace_back(std::move(AttachmentImages));
		}

		// Image Creation for Resolve Attachment
		if (Utils::IsMultisampled(m_Specification))
		{
			std::vector<VulkanImage> ResolveImages;
			ResolveImages.reserve(VulkanSwapChain::MaxFramesInFlight);

			for (int i = 0; i < VulkanSwapChain::MaxFramesInFlight; i++)
			{
				ImageSpecification spec;
				spec.Width = m_Specification.Width;
				spec.Height = m_Specification.Height;
				spec.Samples = 1;
				spec.Format = ImageFormat::RGBA8_SRGB;
				spec.Usage = ImageUsage::Attachment;

				auto& resolveColorImage = ResolveImages.emplace_back(spec);
				resolveColorImage.Invalidate();

				// Resolve Transition
				VkCommandBuffer barrierCmd = device->GetCommandBuffer();

				Utils::InsertImageMemoryBarrier(barrierCmd, resolveColorImage.GetVulkanImageInfo().Image,
					VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

				device->FlushCommandBuffer(barrierCmd);
			}

			m_ColorAttachments.emplace_back(std::move(ResolveImages));
		}

		// Image Creation for Depth Attachment
		if (m_DepthAttachmentSpecification)
		{
			m_DepthAttachment.reserve(VulkanSwapChain::MaxFramesInFlight);

			for (int i = 0; i < VulkanSwapChain::MaxFramesInFlight; i++)
			{
				ImageSpecification spec;
				spec.Width = m_Specification.Width;
				spec.Height = m_Specification.Height;
				spec.Samples = m_Specification.Samples;
				spec.Format = m_DepthAttachmentSpecification.ImageFormat;
				spec.Usage = ImageUsage::Attachment;

				auto& depthImage = m_DepthAttachment.emplace_back(spec);
				depthImage.Invalidate();
			}
		}
	}

	void VulkanFramebuffer::CreateFramebuffer(VkRenderPass renderPass)
	{
		auto device = VulkanContext::GetCurrentDevice();

		if (!m_Framebuffers.empty())
			Release();

		m_Framebuffers.resize(VulkanSwapChain::MaxFramesInFlight);

		for (int i = 0; i < VulkanSwapChain::MaxFramesInFlight; i++)
		{
			std::vector<VkImageView> Attachments;

			for (const auto& attachment : m_ColorAttachments)
				Attachments.push_back(attachment[i].GetVulkanImageInfo().ImageView);

			if (HasDepthAttachment())
				Attachments.push_back(m_DepthAttachment[i].GetVulkanImageInfo().ImageView);

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
			framebufferInfo.pAttachments = Attachments.data();
			framebufferInfo.width = m_Specification.Width;
			framebufferInfo.height = m_Specification.Height;
			framebufferInfo.layers = 1;

			VK_CHECK_RESULT(vkCreateFramebuffer(device->GetVulkanDevice(), &framebufferInfo, nullptr, &m_Framebuffers[i]), "Failed to Create Framebuffer!");
		}
	}

	void VulkanFramebuffer::Release()
	{
		auto device = VulkanContext::GetCurrentDevice();

		for (auto& Framebuffer : m_Framebuffers)
			vkDestroyFramebuffer(device->GetVulkanDevice(), Framebuffer, nullptr);

		m_Framebuffers.clear();
	}

	void VulkanFramebuffer::Resize(uint32_t width, uint32_t height)
	{
		m_Specification.Width = width;
		m_Specification.Height = height;

		for (auto& fbImages : m_ColorAttachments)
			fbImages.clear();

		m_ColorAttachments.clear();
		m_DepthAttachment.clear();
		Invalidate();
	}

}