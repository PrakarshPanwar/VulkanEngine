#pragma once
#include "VulkanDevice.h"
#include "VulkanFramebuffer.h"
#include "VulkanRenderCommandBuffer.h"
#include "VulkanCore/Renderer/RenderPass.h"

namespace VulkanCore {

	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass(const RenderPassSpecification& spec);
		~VulkanRenderPass();

		void Invalidate();
		void RecreateFramebuffers(uint32_t width, uint32_t height) override;
		void SetColorAttachment(uint32_t index, const std::vector<std::shared_ptr<Image2D>>& colorImages) override;
		void SetDepthAttachment(const std::vector<std::shared_ptr<Image2D>>& depthImages) override;

		void Begin(const std::shared_ptr<VulkanRenderCommandBuffer>& beginCmd);
		void End(const std::shared_ptr<VulkanRenderCommandBuffer>& endCmd);

		VkRenderPass GetVulkanRenderPass() const { return m_RenderPass; }
		const RenderPassSpecification& GetSpecification() const override { return m_Specification; }
	private:
		void Release();
	private:
		RenderPassSpecification m_Specification;

		std::vector<VkClearValue> m_ClearValues;
		std::vector<VkAttachmentDescription2> m_AttachmentDescriptions;
		VkRenderPass m_RenderPass = nullptr;
	};

}
