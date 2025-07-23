#pragma once
#include "Resource.h"
#include "Framebuffer.h"

namespace VulkanCore {

	enum class AttachmentLoadOp
	{
		DontCare,
		Load,
		Clear
	};

	enum class AttachmentStoreOp
	{
		DontCare,
		Store
	};

	struct RenderPassSpecification
	{
		std::shared_ptr<Framebuffer> TargetFramebuffer;
		std::vector<AttachmentLoadOp> ColorLoadOps;
		std::vector<AttachmentStoreOp> ColorStoreOps;
		AttachmentLoadOp DepthLoadOp = AttachmentLoadOp::Clear;
		AttachmentStoreOp DepthStoreOp = AttachmentStoreOp::Store;
	};

	class RenderPass : public Resource
	{
	public:
		virtual void RecreateFramebuffers(uint32_t width, uint32_t height) = 0;
		virtual void SetColorAttachment(uint32_t index, const std::vector<std::shared_ptr<Image2D>>& colorImages) = 0;
		virtual void SetDepthAttachment(const std::vector<std::shared_ptr<Image2D>>& depthImages) = 0;

		virtual const RenderPassSpecification& GetSpecification() const = 0;
	};

}
