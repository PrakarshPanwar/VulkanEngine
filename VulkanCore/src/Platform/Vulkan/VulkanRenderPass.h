#pragma once
#include "VulkanDevice.h"
#include "VulkanFramebuffer.h"

namespace VulkanCore {

	struct RenderPassSpecification
	{
		std::shared_ptr<VulkanFramebuffer> TargetFramebuffer;
	};

	class VulkanRenderPass
	{
	public:
		VulkanRenderPass(const RenderPassSpecification& spec);

		void Invalidate();
	private:
		RenderPassSpecification m_Specification;

		std::vector<VkAttachmentDescription> m_AttachmentDescriptions;
		std::vector<VkAttachmentReference> m_AttachmentReferences;

		VkRenderPass m_RenderPass = nullptr;
	};

}