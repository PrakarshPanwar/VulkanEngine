#pragma once
#include "Platform/Vulkan/VulkanDevice.h"
#include "Platform/Vulkan/VulkanImage.h"

namespace VulkanCore {

	class SceneRenderer
	{
	public:
		SceneRenderer();
		~SceneRenderer();

		void Init();
		void Release();
		void RecreateScene();

		static SceneRenderer* GetSceneRenderer() { return s_Instance; }

		// TODO: Use struct SceneRendererData
		inline VkCommandBuffer GetCommandBuffer(uint32_t index) { return m_SceneCommandBuffers[index]; }
		inline VkFramebuffer GetFramebuffer(uint32_t index) { return m_SceneFramebuffers[index]; }
		inline VkRenderPass GetRenderPass() { return m_SceneRenderPass; }
		inline const VulkanImage& GetImage(uint32_t index) const { return m_SceneReadImages[index]; }
	private:
		void CreateCommandBuffers();
		void CreateImagesandViews();
		void CreateRenderPass();
		void CreateFramebuffers();
	private: // TODO: Use VulkanFramebuffer and VulkanRenderPass along with struct SceneRendererData
		std::vector<VulkanImage> m_SceneReadImages, m_SceneColorImages, m_SceneDepthImages;
		std::vector<VkFramebuffer> m_SceneFramebuffers;
		std::vector<VkCommandBuffer> m_SceneCommandBuffers;
		VkRenderPass m_SceneRenderPass;

		// TODO: Could be multiple instances but for now only one is required
		static SceneRenderer* s_Instance;
	};

}