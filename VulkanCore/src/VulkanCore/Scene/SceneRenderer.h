#pragma once
#include "VulkanCore/Renderer/EditorCamera.h"
#include "VulkanCore/Renderer/ComputePipeline.h"
#include "VulkanCore/Renderer/UniformBuffer.h"
#include "VulkanCore/Renderer/StorageBuffer.h"
#include "VulkanCore/Renderer/VertexBuffer.h"
#include "VulkanCore/Renderer/RenderCommandBuffer.h"
#include "VulkanCore/Renderer/AccelerationStructure.h"
#include "Platform/Vulkan/VulkanVertexBuffer.h"
#include "Platform/Vulkan/VulkanRayTracingPipeline.h"

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
		void CreateRayTraceResources();

		void SetActiveScene(std::shared_ptr<Scene> scene);
		void SetViewportSize(uint32_t width, uint32_t height);
		void SetBuffersData(EditorCamera& camera);
		void RasterizeScene();
		void TraceScene();
		void RenderLights();
		void SubmitMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const glm::mat4& transform);
		void SubmitTransparentMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const glm::mat4& transform);
		void SubmitRayTracedMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const glm::mat4& transform);
		void UpdateMeshInstanceData(std::shared_ptr<Mesh> mesh, std::shared_ptr<MaterialAsset> materialAsset);
		void UpdateSkybox(const std::string& filepath);

		static SceneRenderer* GetSceneRenderer() { return s_Instance; }
		static inline VkDescriptorSet GetTextureCubeID() { return s_Instance->m_SkyboxTextureID; }
		static std::shared_ptr<RenderCommandBuffer> GetRenderCommandBuffer() { return s_Instance->m_SceneCommandBuffer; }
		static void SetSkybox(const std::string& filepath);

		inline glm::ivec2 GetViewportSize() const { return m_ViewportSize; }
		std::shared_ptr<Image2D> GetFinalPassImage(uint32_t index) const;
		inline VkDescriptorSet GetSceneImage(uint32_t index) const { return m_SceneImages[index]; }
	private:
		void CreateCommandBuffers();
		void CreatePipelines();
		void CreateResources();
		void CreateMaterials();
		void RecreateMaterials();
		void RecreatePipelines();

		void GeometryPass();
		void CompositePass();
		void BloomCompute();
		void RayTracePass();
		void ResetDrawCommands();
	private:
		struct DrawCommand
		{
			std::shared_ptr<Mesh> MeshInstance;
			std::shared_ptr<Material> MaterialInstance;
			std::shared_ptr<VertexBuffer> TransformBuffer;
			uint32_t SubmeshIndex;
			uint32_t InstanceCount;
		};

		struct TraceCommand
		{
			std::shared_ptr<Mesh> MeshInstance;
			std::shared_ptr<MaterialAsset> MaterialInstance;
			uint32_t SubmeshIndex;
			uint32_t InstanceCount;
		};

		struct MeshTransform
		{
			MeshTransform()
			{
				Transforms.reserve(10);
			}

			std::vector<TransformData> Transforms;
			std::shared_ptr<VertexBuffer> TransformBuffer = std::make_shared<VulkanVertexBuffer>(10 * sizeof(VkTransformMatrixKHR));
		};

		struct LodAndMode
		{
			float LOD = 1.0f;
			float Mode = 1.0f; // 0->PreFilter, 1->Downsample, 2->Upsample-First, 3->Upsample
		};

		struct SceneSettings
		{
			float Exposure = 1.0f;
			float DirtIntensity = 5.0f;
		};

		struct BloomParams
		{
			float Threshold = 1.0f;
			float Knee = 0.5f;
		};

		struct SkyboxSettings
		{
			float Intensity = 0.05f;
			float LOD = 0.0f;
		};
	private:
		std::shared_ptr<Scene> m_Scene;

		std::shared_ptr<RenderCommandBuffer> m_SceneCommandBuffer;
		std::shared_ptr<AccelerationStructure> m_SceneAccelerationStructure;
		std::shared_ptr<Framebuffer> m_SceneFramebuffer;
		std::vector<VkDescriptorSet> m_SceneImages;

		// Pipelines
		std::shared_ptr<Pipeline> m_GeometryPipeline;
		std::shared_ptr<Pipeline> m_LightPipeline;
		std::shared_ptr<Pipeline> m_CompositePipeline;
		std::shared_ptr<Pipeline> m_SkyboxPipeline;
		std::shared_ptr<ComputePipeline> m_BloomPipeline;
		std::shared_ptr<RayTracingPipeline> m_RayTracingPipeline;

		// TODO: In future we have to setup Material Table
		// Material Resources
		// Material per Shader set
		std::shared_ptr<Material> m_GeometryMaterial;
		std::shared_ptr<Material> m_PointLightShaderMaterial;
		std::shared_ptr<Material> m_SpotLightShaderMaterial;
		std::shared_ptr<Material> m_CompositeShaderMaterial;
		std::shared_ptr<Material> m_SkyboxMaterial;

		// Bloom Materials
		std::shared_ptr<Material> m_BloomPrefilterShaderMaterial;
		std::vector<std::shared_ptr<Material>> m_BloomPingShaderMaterials;
		std::vector<std::shared_ptr<Material>> m_BloomPongShaderMaterials;
		std::shared_ptr<Material> m_BloomUpsampleFirstShaderMaterial;
		std::vector<std::shared_ptr<Material>> m_BloomUpsampleShaderMaterials;

		// Ray Tracing Pipeline Material
		std::shared_ptr<Material> m_RayTracingMaterial;

		// Resources
		std::vector<std::shared_ptr<UniformBuffer>> m_UBCamera;
		std::vector<std::shared_ptr<UniformBuffer>> m_UBPointLight;
		std::vector<std::shared_ptr<UniformBuffer>> m_UBSpotLight;

		std::vector<std::shared_ptr<StorageBuffer>> m_SBVertexData;
		std::vector<std::shared_ptr<StorageBuffer>> m_SBIndexData;

		std::vector<glm::vec4> m_PointLightPositions, m_SpotLightPositions;

		std::vector<std::shared_ptr<Image2D>> m_BloomTextures;
		std::vector<std::shared_ptr<Image2D>> m_SceneRenderTextures;
		std::vector<std::shared_ptr<Image2D>> m_SceneRTOutputImages;

		std::shared_ptr<Texture2D> m_BloomDirtTexture;
		std::shared_ptr<Texture2D> m_PointLightTextureIcon, m_SpotLightTextureIcon;

		// Skybox Resources
		std::shared_ptr<TextureCube> m_CubemapTexture, m_IrradianceTexture, m_PrefilteredTexture;
		std::shared_ptr<Image2D> m_BRDFTexture;
		std::shared_ptr<VertexBuffer> m_SkyboxVBData;
		SkyboxSettings m_SkyboxSettings;
		VkDescriptorSet m_SkyboxTextureID;

		std::map<MeshKey, DrawCommand> m_MeshDrawList;
		std::map<MeshKey, TraceCommand> m_MeshTraceList;
		std::map<MeshKey, MeshTransform> m_MeshTransformMap;

		glm::ivec2 m_ViewportSize = { 1920, 1080 };
		glm::uvec2 m_BloomMipSize;

		SceneSettings m_SceneSettings;
		LodAndMode m_LodAndMode;
		BloomParams m_BloomParams;

		// TODO: Could be multiple instances but for now only one is required
		static SceneRenderer* s_Instance;
	};

}
