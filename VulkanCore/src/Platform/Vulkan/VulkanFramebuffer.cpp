#include "vulkanpch.h"
#include "VulkanFramebuffer.h"

#include "VulkanSwapChain.h"
#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Renderer.h"

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

		static uint32_t CalculateMipCount(uint32_t width, uint32_t height)
		{
			return (uint32_t)std::_Floor_of_log_2(std::max(width, height)) + 1;
		}

		static bool IsMultisampled(FramebufferSpecification spec)
		{
			return spec.Samples > 1;
		}

		static bool IsMultisampled(ImageSpecification spec)
		{
			return spec.Samples > 1;
		}

	}

	uint32_t VulkanFramebuffer::s_InstanceCount = 0;

	VulkanFramebuffer::VulkanFramebuffer(const FramebufferSpecification& spec)
		: m_Specification(spec)
	{
		for (auto specification : m_Specification.Attachments.Attachments)
		{
			if (Utils::IsDepthFormat(specification.ImgFormat))
				m_DepthAttachmentSpecification = specification;
			else
				m_ColorAttachmentSpecifications.emplace_back(specification);
		}

		Renderer::Submit([this]
		{
			Invalidate();
		});

		s_InstanceCount++;
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{
		if (m_Framebuffers.at(0) == nullptr)
			return;

		VK_CORE_TRACE("Framebuffers Instances: {}", s_InstanceCount);
		Release();
	}

	const std::vector<std::shared_ptr<Image2D>>& VulkanFramebuffer::GetAttachment(bool resolve, uint32_t index) const
	{
		bool multisampled = Utils::IsMultisampled(m_Specification);
		uint32_t attachmentSize = static_cast<uint32_t>(m_ColorAttachmentSpecifications.size());
		return m_ColorAttachments[resolve && multisampled ? attachmentSize + index : index];
	}

	const std::vector<std::shared_ptr<Image2D>>& VulkanFramebuffer::GetDepthAttachment(bool resolve) const
	{
		bool multisampled = Utils::IsMultisampled(m_Specification);
		return resolve && multisampled ? m_DepthAttachmentResolve : m_DepthAttachment;
	}

	void VulkanFramebuffer::Invalidate()
	{
		auto device = VulkanContext::GetCurrentDevice();

		bool multisampled = Utils::IsMultisampled(m_Specification);
		uint32_t attachmentSize = static_cast<uint32_t>(multisampled ? (m_ColorAttachmentSpecifications.size() + 1) : m_ColorAttachmentSpecifications.size());
		m_ColorAttachments.reserve(attachmentSize);

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		// Image Creation for Color Attachments
		for (auto& attachment : m_ColorAttachmentSpecifications)
		{
			std::vector<std::shared_ptr<Image2D>> AttachmentImages;
			AttachmentImages.reserve(framesInFlight);

			// Adding Images in Flight
			for (uint32_t i = 0; i < framesInFlight; ++i)
			{
				ImageSpecification spec;
				spec.DebugName = "Framebuffer Color Attachment";
				spec.Width = m_Specification.Width;
				spec.Height = m_Specification.Height;
				spec.Samples = m_Specification.Samples;
				spec.Format = attachment.ImgFormat;
				spec.Usage = ImageUsage::Attachment;

				auto attachmentColorImage = std::make_shared<VulkanImage>(spec);
				attachmentColorImage->Invalidate();

				AttachmentImages.push_back(attachmentColorImage);

				if (!multisampled)
				{
					VkCommandBuffer barrierCmd = device->GetCommandBuffer();

					Utils::InsertImageMemoryBarrier(barrierCmd, 
						attachmentColorImage->GetVulkanImageInfo().Image,
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
		if (multisampled)
		{
			for (auto& attachment : m_ColorAttachmentSpecifications)
			{
				std::vector<std::shared_ptr<Image2D>> ResolveImages;
				ResolveImages.reserve(framesInFlight);

				for (uint32_t i = 0; i < framesInFlight; ++i)
				{
					ImageSpecification spec;
					spec.DebugName = "Framebuffer Color Resolve";
					spec.Width = m_Specification.Width;
					spec.Height = m_Specification.Height;
					spec.Samples = 1;
					spec.Transfer = m_Specification.Transfer;
					spec.Format = attachment.ImgFormat;
					spec.Usage = ImageUsage::Attachment;

					auto resolveColorImage = std::make_shared<VulkanImage>(spec);
					resolveColorImage->Invalidate();

					ResolveImages.push_back(resolveColorImage);

					// Resolve Transition
					VkCommandBuffer barrierCmd = device->GetCommandBuffer();

					Utils::InsertImageMemoryBarrier(barrierCmd, resolveColorImage->GetVulkanImageInfo().Image,
						VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
						VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
						VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

					device->FlushCommandBuffer(barrierCmd);
				}

				m_ColorAttachments.emplace_back(std::move(ResolveImages));
			}
		}

		// Image Creation for Depth Attachment
		if (m_DepthAttachmentSpecification)
		{
			m_DepthAttachment.reserve(framesInFlight);

			for (uint32_t i = 0; i < framesInFlight; ++i)
			{
				ImageSpecification spec;
				spec.DebugName = "Framebuffer Depth Attachment";
				spec.Width = m_Specification.Width;
				spec.Height = m_Specification.Height;
				spec.Samples = m_Specification.Samples;
				spec.Format = m_DepthAttachmentSpecification.ImgFormat;
				spec.Usage = ImageUsage::Attachment;

				auto depthImage = std::make_shared<VulkanImage>(spec);
				depthImage->Invalidate();

				m_DepthAttachment.push_back(depthImage);
			}

			if (multisampled && m_Specification.ReadDepthTexture)
			{
				m_DepthAttachmentResolve.reserve(framesInFlight);

				for (uint32_t i = 0; i < framesInFlight; ++i)
				{
					ImageSpecification spec;
					spec.DebugName = "Framebuffer Depth Resolve";
					spec.Width = m_Specification.Width;
					spec.Height = m_Specification.Height;
					spec.Samples = 1;
					spec.Format = m_DepthAttachmentSpecification.ImgFormat;
					spec.Usage = ImageUsage::Attachment;

					auto depthResolveImage = std::make_shared<VulkanImage>(spec);
					depthResolveImage->Invalidate();

					m_DepthAttachmentResolve.push_back(depthResolveImage);

					// Depth Resolve Transition
					VkCommandBuffer barrierCmd = device->GetCommandBuffer();

					Utils::InsertImageMemoryBarrier(barrierCmd, depthResolveImage->GetVulkanImageInfo().Image,
						VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
						VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
						VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 });

					device->FlushCommandBuffer(barrierCmd);
				}
			}
		}
	}

	void VulkanFramebuffer::CreateFramebuffer(VkRenderPass renderPass)
	{
		auto device = VulkanContext::GetCurrentDevice();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		if (!m_Framebuffers.empty())
			Release();

		m_Framebuffers.resize(framesInFlight);

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			std::vector<VkImageView> Attachments;

			for (const auto& colorAttachment : m_ColorAttachments)
			{
				auto attachment = std::static_pointer_cast<VulkanImage>(colorAttachment[i]);
				Attachments.push_back(attachment->GetVulkanImageInfo().ImageView);
			}

			if (HasDepthAttachment() && m_Specification.ReadDepthTexture)
			{
				auto depthAttachment = std::static_pointer_cast<VulkanImage>(m_DepthAttachment[i]);
				Attachments.push_back(depthAttachment->GetVulkanImageInfo().ImageView);

				auto depthAttachmentResolve = std::static_pointer_cast<VulkanImage>(m_DepthAttachmentResolve[i]);
				Attachments.push_back(depthAttachmentResolve->GetVulkanImageInfo().ImageView);
			}
			else if (HasDepthAttachment())
			{
				auto depthAttachment = std::static_pointer_cast<VulkanImage>(m_DepthAttachment[i]);
				Attachments.push_back(depthAttachment->GetVulkanImageInfo().ImageView);
			}

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
		Renderer::SubmitResourceFree([framebuffers = m_Framebuffers]
		{
			auto device = VulkanContext::GetCurrentDevice();

			for (auto& framebuffer : framebuffers)
				vkDestroyFramebuffer(device->GetVulkanDevice(), framebuffer, nullptr);
		});
	}

	void VulkanFramebuffer::Resize(uint32_t width, uint32_t height)
	{
		auto device = VulkanContext::GetCurrentDevice();

		m_Specification.Width = width;
		m_Specification.Height = height;

		VkCommandBuffer barrierCmd = device->GetCommandBuffer();

		for (auto& fbImages : m_ColorAttachments)
		{
			for (auto& fbImage : fbImages)
			{
				bool multisampled = Utils::IsMultisampled(fbImage->GetSpecification());
				auto vulkanFBImage = std::static_pointer_cast<VulkanImage>(fbImage);
				vulkanFBImage->Resize(width, height);

				if (!multisampled)
				{
					Utils::InsertImageMemoryBarrier(barrierCmd, vulkanFBImage->GetVulkanImageInfo().Image,
						VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
						VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
						VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
				}
			}
		}

		for (auto& depthImage : m_DepthAttachment)
			depthImage->Resize(width, height);

		for (auto& depthResolveImage : m_DepthAttachmentResolve)
		{
			auto vulkanDepthResolveImage = std::static_pointer_cast<VulkanImage>(depthResolveImage);
			depthResolveImage->Resize(width, height);

			Utils::InsertImageMemoryBarrier(barrierCmd, vulkanDepthResolveImage->GetVulkanImageInfo().Image,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 });
		}

		device->FlushCommandBuffer(barrierCmd);
	}

}
