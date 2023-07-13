#pragma once
#include "VulkanDevice.h"
#include "VulkanFramebuffer.h"
#include "VulkanRenderCommandBuffer.h"
#include "VulkanCore/Renderer/RenderPass.h"

namespace VulkanCore {

	// TODO: To read Depth Image we have to also create its resolve form
	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass(const RenderPassSpecification& spec);
		~VulkanRenderPass();

		void Invalidate();
		void RecreateFramebuffers(uint32_t width, uint32_t height) override;

		void Begin(const std::shared_ptr<VulkanRenderCommandBuffer>& beginCmd);
		void End(const std::shared_ptr<VulkanRenderCommandBuffer>& endCmd);

		inline VkRenderPass GetVulkanRenderPass() const { return m_RenderPass; }
		inline const RenderPassSpecification& GetSpecification() const override { return m_Specification; }
	private:
		RenderPassSpecification m_Specification;

		std::vector<VkClearValue> m_ClearValues;
		std::vector<VkAttachmentDescription> m_AttachmentDescriptions;
		VkRenderPass m_RenderPass = nullptr;
	};

}