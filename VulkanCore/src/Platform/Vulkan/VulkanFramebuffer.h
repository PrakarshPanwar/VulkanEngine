#pragma once
#include "VulkanImage.h"

namespace VulkanCore {

	struct FramebufferTextureSpecification
	{
		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification(ImageFormat format)
			: ImageFormat(format) {}

		ImageFormat ImageFormat = ImageFormat::None;

		operator bool() const { return ImageFormat == ImageFormat::None ? false : true; }
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

		inline const std::vector<VkFramebuffer>& GetVulkanFramebuffers() const { return m_Framebuffers; }
		inline const FramebufferSpecification& GetSpecification() const { return m_Specification; }
		inline const std::vector<FramebufferTextureSpecification>& GetColorAttachments() const { return m_ColorAttachmentSpecifications; }
		inline const FramebufferTextureSpecification& GetDepthAttachment() const { return m_DepthAttachmentSpecification; }
		inline bool HasDepthAttachment() { return m_DepthAttachmentSpecification; }
		const std::vector<VulkanImage>& GetResolveAttachment() const;

		void Invalidate();
		void CreateFramebuffer(VkRenderPass renderPass); // TODO: This could be removed from here
		void Release();
		void Resize(uint32_t width, uint32_t height);
	private:
		FramebufferSpecification m_Specification;

		std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
		FramebufferTextureSpecification m_DepthAttachmentSpecification;

		std::vector<std::vector<VulkanImage>> m_ColorAttachments;
		std::vector<VulkanImage> m_DepthAttachment;

		std::vector<VkFramebuffer> m_Framebuffers;

		static uint32_t s_InstanceCount;
	};

}