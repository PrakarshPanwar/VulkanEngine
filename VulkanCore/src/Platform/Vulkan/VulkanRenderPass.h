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
		~VulkanRenderPass();

		void Invalidate();
		void RecreateFramebuffers(uint32_t width, uint32_t height);

		inline VkRenderPass GetRenderPass() const { return m_RenderPass; }
		inline const RenderPassSpecification& GetSpecification() const { return m_Specification; }
	private:
		RenderPassSpecification m_Specification;

		VkRenderPass m_RenderPass = nullptr;
	};

}