#pragma once
#include "Platform/Vulkan/VulkanDevice.h"
#include "Platform/Vulkan/VulkanSwapChain.h"
#include "Platform/Vulkan/VulkanRenderPass.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "Platform/Vulkan/VulkanComputePipeline.h"
#include "Platform/Vulkan/VulkanTexture.h"
#include "Platform/Vulkan/VulkanBuffer.h"
#include "VulkanCore/Renderer/EditorCamera.h"

#include <glm/glm.hpp>
#include "Scene.h"

namespace VulkanCore {

	class SceneRenderer
	{
	public:
		SceneRenderer(std::shared_ptr<Scene> scene);
		~SceneRenderer();

		void Init();
		void Release();
		void RecreateScene();
		void OnImGuiRender();

		void SetViewportSize(uint32_t width, uint32_t height) { m_ViewportSize.x = width; m_ViewportSize.y = height; }
		void RenderScene(EditorCamera& camera);

		static SceneRenderer* GetSceneRenderer() { return s_Instance; }

		// TODO: Use struct SceneRendererData
		inline VkCommandBuffer GetCommandBuffer(uint32_t index) { return m_SceneCommandBuffers[index]; }
		inline std::shared_ptr<VulkanFramebuffer> GetFramebuffer() { return m_SceneFramebuffer; }
		inline std::shared_ptr<VulkanRenderPass> GetRenderPass() { return m_SceneRenderPass; }
		inline VkFramebuffer GetFinalVulkanFramebuffer(uint32_t index) { return m_SceneFramebuffer->GetVulkanFramebuffers()[index]; }
		inline VkRenderPass GetVulkanRenderPass() { return m_SceneRenderPass->GetRenderPass(); }
		inline const VulkanImage& GetFinalPassImage(uint32_t index) { return m_SceneFramebuffer->GetResolveAttachment()[index]; }
	private:
		void CreateCommandBuffers();
		void CreatePipelines();
		void CreateDescriptorSets();

		void GeometryPass();
		void CompositePass();
	private:
		std::shared_ptr<Scene> m_Scene;

		std::vector<VkCommandBuffer> m_SceneCommandBuffers;
		std::shared_ptr<VulkanFramebuffer> m_SceneFramebuffer;
		std::shared_ptr<VulkanRenderPass> m_SceneRenderPass;

		// Pipelines
		std::shared_ptr<VulkanPipeline> m_GeometryPipeline;
		std::shared_ptr<VulkanPipeline> m_PointLightPipeline;
		std::shared_ptr<VulkanPipeline> m_CompositePipeline;
		std::shared_ptr<VulkanComputePipeline> m_ComputeDemoPipeline;

		// TODO: Setup VulkanMaterial to do this
		// Descriptor Sets
		std::vector<VkDescriptorSet> m_GeometryDescriptorSets;
		std::vector<VkDescriptorSet> m_PointLightDescriptorSets;
		std::vector<VkDescriptorSet> m_CompositeDescriptorSets;
		std::vector<VkDescriptorSet> m_ComputeDemoDescriptorSets;

		// TODO: In future we could have to setup a Material Table and Instanced Rendering
		// Material Resources
		std::vector<std::unique_ptr<VulkanBuffer>> m_CameraUBs{ VulkanSwapChain::MaxFramesInFlight };
		std::vector<std::unique_ptr<VulkanBuffer>> m_PointLightUBs{ VulkanSwapChain::MaxFramesInFlight };
		std::vector<std::unique_ptr<VulkanBuffer>> m_ExposureUBs{ VulkanSwapChain::MaxFramesInFlight };

		std::shared_ptr<VulkanTexture> m_DiffuseMap, m_NormalMap, m_SpecularMap,
			m_DiffuseMap2, m_NormalMap2, m_SpecularMap2,
			m_DiffuseMap3, m_NormalMap3, m_SpecularMap3;

		glm::ivec2 m_ViewportSize;
		float m_Exposure = 1.0f;

		// TODO: Could be multiple instances but for now only one is required
		static SceneRenderer* s_Instance;
	};

}