#pragma once
#include "Image.h"

#include <glm/glm.hpp>

namespace VulkanCore {

	enum class BlendFactor
	{
		None = 0,
		Zero,
		One,
		SrcColor,
		OneMinusSrcColor,
		SrcAlpha,
		OneMinusSrcAlpha
	};

	struct FramebufferTextureSpecification
	{
		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification(ImageFormat format)
			: ImgFormat(format) {}

		FramebufferTextureSpecification(ImageFormat format, BlendFactor srcColorBlendFactor, BlendFactor dstColorBlendFactor, float clearColor = 0.0f)
			: ImgFormat(format), SrcColorBlendFactor(srcColorBlendFactor), DstColorBlendFactor(dstColorBlendFactor), ClearColor(clearColor) {}

		ImageFormat ImgFormat = ImageFormat::None;
		BlendFactor SrcColorBlendFactor = BlendFactor::One, DstColorBlendFactor = BlendFactor::Zero;
		float ClearColor = 0.0f;

		operator bool() const { return ImgFormat != ImageFormat::None; }
		const bool IsBlended() const
		{
			return SrcColorBlendFactor != BlendFactor::One || DstColorBlendFactor != BlendFactor::Zero;
		}
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
		uint32_t Width = 1, Height = 1;
		FramebufferAttachmentSpecification Attachments;
		uint32_t Samples = 1, Layers = 1;
		bool Transfer = false, ReadDepthTexture = false;
	};

	class Framebuffer : public Resource
	{
	public:
		virtual const std::vector<std::shared_ptr<Image2D>>& GetAttachment(uint32_t index = 0, bool resolved = true) const = 0;
		virtual const std::vector<std::shared_ptr<Image2D>>& GetDepthAttachment(bool resolved = true) const = 0;
		virtual const std::vector<FramebufferTextureSpecification>& GetColorAttachmentsTextureSpec() const = 0;
		virtual const FramebufferTextureSpecification& GetDepthAttachmentTextureSpec() const = 0;

		virtual const FramebufferSpecification& GetSpecification() const = 0;
	};

}
