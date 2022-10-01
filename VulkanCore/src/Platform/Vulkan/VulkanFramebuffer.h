#pragma once
#include "VulkanDevice.h"

namespace VulkanCore {

	enum class FramebufferTextureFormat
	{
		None,

		RGBA,
		RGBA16F,
		RGBA32F,

		DEPTH24STENCIL8,
		DEPTH16F,
		DEPTH32F
	};

	struct FramebufferTextureSpecification
	{
		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification(FramebufferTextureFormat format)
			: TextureFormat(format) {}

		FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;
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

		void Invalidate();
	private:
		FramebufferSpecification m_Specification;

		std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
		FramebufferTextureSpecification m_DepthAttachmentSpecification;

		std::vector<std::vector<VkImageView>> m_ColorAttachments;
		std::vector<VkImageView> m_DepthAttachment;
	};

}