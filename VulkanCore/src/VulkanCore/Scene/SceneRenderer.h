#pragma once
#include "Platform/Vulkan/VulkanDevice.h"
#include "Platform/Vulkan/VulkanSwapChain.h"
#include "Platform/Vulkan/VulkanRenderPass.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "Platform/Vulkan/VulkanComputePipeline.h"
#include "Platform/Vulkan/VulkanTexture.h"
#include "Platform/Vulkan/VulkanBuffer.h"
#include "Platform/Vulkan/VulkanUniformBuffer.h"
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

		inline glm::ivec2 GetViewportSize() const { return m_ViewportSize; }
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
		void BloomCompute();
	private:
		struct LodAndMode
		{
			float LOD = 1.0f;
			float Mode = 1.0f; // 0->PreFilter, 1->Downsample, 2->Upsample-First, 3->Upsample
		};

		struct SceneSettings
		{
			float Exposure = 1.0f;
		};

		struct BloomParams
		{
			float Threshold = 1.0f;
			float Knee = 1.0f;
		};

		struct SkyboxSettings
		{
			float Intensity = 1.0f;
			float LOD = 0.0f;
		};
	private:
		std::shared_ptr<Scene> m_Scene;

		std::vector<VkCommandBuffer> m_SceneCommandBuffers;
		std::shared_ptr<VulkanFramebuffer> m_SceneFramebuffer;
		std::shared_ptr<VulkanRenderPass> m_SceneRenderPass;

		// Pipelines
		std::shared_ptr<VulkanPipeline> m_GeometryPipeline;
		std::shared_ptr<VulkanPipeline> m_PointLightPipeline;
		std::shared_ptr<VulkanPipeline> m_CompositePipeline;
		std::shared_ptr<VulkanPipeline> m_SkyboxPipeline;
		std::shared_ptr<VulkanComputePipeline> m_BloomPipeline;

		// TODO: Setup VulkanMaterial to do this
		// Descriptor Sets
		std::vector<VkDescriptorSet> m_GeometryDescriptorSets;
		std::vector<VkDescriptorSet> m_PointLightDescriptorSets;
		std::vector<VkDescriptorSet> m_CompositeDescriptorSets;
		std::vector<VkDescriptorSet> m_SkyboxDescriptorSets;

		std::vector<VkDescriptorSet> m_BloomPrefilterSets;
		std::vector<std::vector<VkDescriptorSet>> m_BloomPingSets;
		std::vector<std::vector<VkDescriptorSet>> m_BloomPongSets;
		std::vector<VkDescriptorSet> m_BloomUpsampleFirstSets;
		std::vector<std::vector<VkDescriptorSet>> m_BloomUpsampleSets;

		VkDescriptorSet m_BloomDebugImage;

		// TODO: In future we have to setup Material Table and Instanced Rendering
		// Material Resources
		std::vector<VulkanUniformBuffer> m_UBCamera;
		std::vector<VulkanUniformBuffer> m_UBPointLight;
		std::vector<VulkanUniformBuffer> m_UBSceneData;

		std::vector<VulkanImage> m_BloomTextures;
		std::vector<VulkanImage> m_SceneRenderTextures;

		std::shared_ptr<VulkanImage> m_BloomTexture;
		std::shared_ptr<VulkanTexture> m_DiffuseMap, m_NormalMap, m_ARMMap,
			m_DiffuseMap2, m_NormalMap2, m_ARMMap2,
			m_DiffuseMap3, m_NormalMap3, m_ARMMap3;

		// Skybox Resources
		std::shared_ptr<VulkanTextureCube> m_CubemapTexture, m_IrradianceTexture;
#define USE_PRELOADED_BRDF 0
#if USE_PRELOADED_BRDF
		std::shared_ptr<VulkanTexture> m_BRDFTexture;
#else
		std::shared_ptr<VulkanImage> m_BRDFTexture;
#endif
		std::shared_ptr<Mesh> m_SkyboxMesh;
		SkyboxSettings m_SkyboxSettings;

		glm::ivec2 m_ViewportSize;
		glm::uvec2 m_BloomMipSize;

		SceneSettings m_SceneSettings;
		LodAndMode m_LodAndMode;
		BloomParams m_BloomParams;

		// TODO: Could be multiple instances but for now only one is required
		static SceneRenderer* s_Instance;
	};

}