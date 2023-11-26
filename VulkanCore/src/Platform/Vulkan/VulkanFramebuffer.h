#pragma once
#include "VulkanCore/Renderer/Framebuffer.h"
#include "VulkanImage.h"

#include <glm/glm.hpp>

namespace VulkanCore {

	class VulkanFramebuffer : public Framebuffer
	{
	public:
		VulkanFramebuffer(const FramebufferSpecification& spec);
		~VulkanFramebuffer();

		inline const std::vector<VkFramebuffer>& GetVulkanFramebuffers() const { return m_Framebuffers; }
		inline const FramebufferSpecification& GetSpecification() const override { return m_Specification; }
		inline const std::vector<FramebufferTextureSpecification>& GetColorAttachmentsTextureSpec() const override { return m_ColorAttachmentSpecifications; }
		inline const FramebufferTextureSpecification& GetDepthAttachmentTextureSpec() const override { return m_DepthAttachmentSpecification; }
		inline bool HasDepthAttachment() const { return m_DepthAttachmentSpecification; }
		const std::vector<std::shared_ptr<Image2D>>& GetAttachment(bool resolve = false, uint32_t index = 0) const override;
		const std::vector<std::shared_ptr<Image2D>>& GetDepthAttachment(bool resolve = false) const override;

		void Invalidate();
		void CreateFramebuffer(VkRenderPass renderPass);
		void Release();
		void Resize(uint32_t width, uint32_t height);
	private:
		FramebufferSpecification m_Specification;

		std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
		FramebufferTextureSpecification m_DepthAttachmentSpecification;

		std::vector<std::vector<std::shared_ptr<Image2D>>> m_ColorAttachments;
		std::vector<std::shared_ptr<Image2D>> m_DepthAttachment, m_DepthAttachmentResolve;

		std::vector<VkFramebuffer> m_Framebuffers;

		static uint32_t s_InstanceCount;
	};

}
