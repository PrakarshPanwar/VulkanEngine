#pragma once
#include "Image.h"

#include <glm/glm.hpp>

namespace VulkanCore {

	struct FramebufferTextureSpecification
	{
		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification(ImageFormat format)
			: ImgFormat(format) {}

		ImageFormat ImgFormat = ImageFormat::None;

		operator bool() const { return ImgFormat != ImageFormat::None; }
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
		glm::vec4 ClearColor = { 0.01f, 0.01f, 0.01f, 1.0f };
		bool Transfer = false;
		bool ReadDepthTexture = false;
	};

	class Framebuffer : public Resource
	{
	public:
		virtual const FramebufferSpecification& GetSpecification() const = 0;
	};

}
