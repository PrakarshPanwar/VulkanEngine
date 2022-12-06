#pragma once
#include "VulkanDevice.h"
#include "VulkanFramebuffer.h"

namespace VulkanCore {

	struct RenderPassSpecification
	{
		std::shared_ptr<VulkanFramebuffer> TargetFramebuffer;
	};

	// TODO: To read Depth Image we have to also create its resolve form
	class VulkanRenderPass
	{
	public:
		VulkanRenderPass(const RenderPassSpecification& spec);
		~VulkanRenderPass();

		void Invalidate();
		void RecreateFramebuffers(uint32_t width, uint32_t height);

		void Begin(VkCommandBuffer beginCmd);
		void End(VkCommandBuffer endCmd);

		inline VkRenderPass GetRenderPass() const { return m_RenderPass; }
		inline const RenderPassSpecification& GetSpecification() const { return m_Specification; }
	private:
		RenderPassSpecification m_Specification;

		std::vector<VkAttachmentDescription> m_AttachmentDescriptions;
		VkRenderPass m_RenderPass = nullptr;
	};

}