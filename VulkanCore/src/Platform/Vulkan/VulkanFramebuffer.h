#pragma once
#include "VulkanImage.h"

namespace VulkanCore {

	struct FramebufferTextureSpecification
	{
		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification(ImageFormat format)
			: TextureFormat(format) {}

		ImageFormat TextureFormat = ImageFormat::None;

		operator bool() const { return TextureFormat == ImageFormat::None ? false : true; }
	};

	struct FramebufferAttachmentSpecification
	{
		FramebufferAttachmentSpecification() = default;
		FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments)
			: Attachments(attachments) {}

		std::vector<FramebufferTextureSpecification> Attachments;
	};

	struct FramebufferSpecification
	{
		uint32_t Width = 0, Height = 0;
		FramebufferAttachmentSpecification Attachments;
		uint32_t Samples = 1;

		bool SwapChainTarget = false;
	};

	class VulkanFramebuffer
	{
	public:
		VulkanFramebuffer(const FramebufferSpecification& spec);
		~VulkanFramebuffer();

		inline const std::vector<VkFramebuffer>& GetVulkanFramebuffer() const { return m_Framebuffers; }
		inline const FramebufferSpecification& GetSpecification() const { return m_Specification; }
		inline const std::vector<FramebufferTextureSpecification>& GetColorAttachments() const { return m_ColorAttachmentSpecifications; }
		inline const FramebufferTextureSpecification& GetDepthAttachment() const { return m_DepthAttachmentSpecification; }

		void Invalidate();
		void CreateFramebuffer(); // TODO: This could be removed from here
		void Release();
	private:
		FramebufferSpecification m_Specification;

		std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
		FramebufferTextureSpecification m_DepthAttachmentSpecification;

		std::vector<std::vector<VulkanImage>> m_ColorAttachments;
		std::vector<VulkanImage> m_DepthAttachment;

		std::vector<VkFramebuffer> m_Framebuffers;
	};

}