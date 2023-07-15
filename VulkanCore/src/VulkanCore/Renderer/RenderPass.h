#pragma once
#include "Resource.h"
#include "Framebuffer.h"

namespace VulkanCore {

	struct RenderPassSpecification
	{
		std::shared_ptr<Framebuffer> TargetFramebuffer;
	};

	class RenderPass : public Resource
	{
	public:
		virtual void RecreateFramebuffers(uint32_t width, uint32_t height) = 0;
		virtual const RenderPassSpecification& GetSpecification() const = 0;
	};

}
