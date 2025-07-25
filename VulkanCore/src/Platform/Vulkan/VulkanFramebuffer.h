#pragma once
#include "VulkanCore/Renderer/Framebuffer.h"
#include "VulkanImage.h"
#include "VulkanIndexBuffer.h"
#include "VulkanRenderCommandBuffer.h"

#include <glm/glm.hpp>

namespace VulkanCore {

	class VulkanFramebuffer : public Framebuffer
	{
	public:
		VulkanFramebuffer(const FramebufferSpecification& spec);
		~VulkanFramebuffer();

		const std::vector<VkFramebuffer>& GetVulkanFramebuffers() const { return m_Framebuffers; }
		const FramebufferSpecification& GetSpecification() const override { return m_Specification; }
		const std::vector<FramebufferTextureSpecification>& GetColorAttachmentsTextureSpec() const override { return m_ColorAttachmentSpecifications; }
		const FramebufferTextureSpecification& GetDepthAttachmentTextureSpec() const override { return m_DepthAttachmentSpecification; }
		bool HasDepthAttachment() const { return m_DepthAttachmentSpecification; }
		const std::vector<std::shared_ptr<Image2D>>& GetAttachment(uint32_t index = 0, bool resolved = true) const override;
		const std::vector<std::shared_ptr<Image2D>>& GetDepthAttachment(bool resolved = true) const override;

		void Invalidate();
		void CreateFramebuffer(VkRenderPass renderPass);
		void Release();
		void Resize(uint32_t width, uint32_t height, const std::bitset<11> resizeBitFlag);
		void SetColorAttachments(uint32_t index, const std::vector<std::shared_ptr<Image2D>>& colorImages);
		void SetDepthAttachments(const std::vector<std::shared_ptr<Image2D>>& depthImages);

		void* ReadPixel(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, std::shared_ptr<IndexBuffer> imageBuffer, uint32_t index, uint32_t x, uint32_t y);
	private:
		FramebufferSpecification m_Specification;

		std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
		FramebufferTextureSpecification m_DepthAttachmentSpecification;

		std::vector<std::vector<std::shared_ptr<Image2D>>> m_ColorAttachments;
		std::vector<std::shared_ptr<Image2D>> m_DepthAttachment, m_DepthAttachmentResolve;

		std::vector<VkFramebuffer> m_Framebuffers;
	};

}
