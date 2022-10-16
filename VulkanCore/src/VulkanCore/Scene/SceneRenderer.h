#pragma once
#include "Platform/Vulkan/VulkanDevice.h"
#include "Platform/Vulkan/VulkanImage.h"
#include "Platform/Vulkan/VulkanRenderPass.h"

#include <glm/glm.hpp>

namespace VulkanCore {

	class SceneRenderer
	{
	public:
		SceneRenderer();
		~SceneRenderer();

		void Init();
		void Release();
		void RecreateScene();

		void SetViewportSize(uint32_t width, uint32_t height) { m_ViewportSize.x = width; m_ViewportSize.y = height; }

		static SceneRenderer* GetSceneRenderer() { return s_Instance; }

		// TODO: Use struct SceneRendererData
		inline VkCommandBuffer GetCommandBuffer(uint32_t index) { return m_SceneCommandBuffers[index]; }
		inline VkFramebuffer GetVulkanFramebuffer(uint32_t index) { return m_SceneFramebuffer->GetVulkanFramebuffers()[index]; }
		inline VkRenderPass GetVulkanRenderPass() { return m_SceneRenderPass->GetRenderPass(); }
		inline std::shared_ptr<VulkanFramebuffer> GetFramebuffer() { return m_SceneFramebuffer; }
		inline std::shared_ptr<VulkanRenderPass> GetRenderPass() { return m_SceneRenderPass; }
		inline const VulkanImage& GetImage(uint32_t index) { return m_SceneFramebuffer->GetResolveAttachment()[index]; }
	private:
		void CreateCommandBuffers();
		void CreateRenderPasswithFramebuffers();
	private:
		std::vector<VkCommandBuffer> m_SceneCommandBuffers;
		std::shared_ptr<VulkanFramebuffer> m_SceneFramebuffer;
		std::shared_ptr<VulkanRenderPass> m_SceneRenderPass;

		glm::ivec2 m_ViewportSize;

		// TODO: Could be multiple instances but for now only one is required
		static SceneRenderer* s_Instance;
	};

}