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
		virtual const RenderPassSpecification& GetSpecification() const = 0;
	};

}
