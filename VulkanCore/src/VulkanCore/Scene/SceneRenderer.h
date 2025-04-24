#pragma once
#include "VulkanCore/Renderer/EditorCamera.h"
#include "VulkanCore/Renderer/ComputePipeline.h"
#include "VulkanCore/Renderer/UniformBuffer.h"
#include "VulkanCore/Renderer/StorageBuffer.h"
#include "VulkanCore/Renderer/RenderCommandBuffer.h"

#include <glm/glm.hpp>
#include "Scene.h"
#include "PhysicsDebugRenderer.h"

#define VK_FEATURE_GTAO 0

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

		void SetActiveScene(std::shared_ptr<Scene> scene);
		void SetViewportSize(uint32_t width, uint32_t height);
		void RenderScene();
		void RenderLights();
		void SelectionPass();
		void SubmitMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialTable>& materialTable, const glm::mat4& transform);
		void SubmitSelectedMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialTable>& materialTable , const glm::mat4& transform, uint32_t entityID);
		void SubmitTransparentMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialTable>& materialTable, const glm::mat4& transform);
		void SubmitPhysicsMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const glm::mat4& transform);
		void UpdateMeshInstanceData(std::shared_ptr<Mesh> mesh, std::shared_ptr<MaterialTable> materialTable);
		void UpdateSkybox(AssetHandle skyTextureHandle);

		inline int GetHoveredEntity() const { return m_HoveredEntity; }
		inline glm::ivec2 GetViewportSize() const { return m_ViewportSize; }
		std::shared_ptr<Image2D> GetFinalPassImage(uint32_t index) const;
		inline VkDescriptorSet GetSceneImage(uint32_t index) const { return m_SceneImages[index]; }
		inline const EditorCamera& GetEditorCamera() const { return m_SceneEditorData.CameraData; }

		static SceneRenderer* GetSceneRenderer() { return s_Instance; }
		static inline VkDescriptorSet GetTextureCubeID() { return s_Instance->m_SkyboxTextureID; }
		static std::shared_ptr<RenderCommandBuffer> GetRenderCommandBuffer() { return s_Instance->m_SceneCommandBuffer; }
		static void SetSkybox(AssetHandle skyTextureHandle);
		void SetSceneEditorData(const SceneEditorData& sceneEditorData) { m_SceneEditorData = sceneEditorData; }

		struct MeshKey
		{
			uint64_t MeshHandle;
			uint64_t MaterialHandle;
			uint32_t SubmeshIndex;

			bool operator==(const MeshKey& other)
			{
				return MeshHandle == other.MeshHandle &&
					SubmeshIndex == other.SubmeshIndex &&
					MaterialHandle == other.MaterialHandle;
			}

			bool operator<(const MeshKey& other) const
			{
				if (MeshHandle < other.MeshHandle)
					return true;

				if (MeshHandle > other.MeshHandle)
					return false;

				if (SubmeshIndex < other.SubmeshIndex)
					return true;

				if (SubmeshIndex > other.SubmeshIndex)
					return false;

				if (MaterialHandle < other.MaterialHandle)
					return true;

				if (MaterialHandle > other.MaterialHandle)
					return false;
				
				return false;
			}
		};
	private:
		void CreateCommandBuffers();
		void CreatePipelines();
		void CreateResources();
		void CreateMaterials();
		void RecreateMaterials();
		void RecreatePipelines();

		std::vector<glm::vec4> GetFrustumCornersWorldSpace();

#define SHADOW_MAP_CASCADE_COUNT 4
		void UpdateCascadeMap();

		void ShadowPass();
		void GeometryPass();
		void CompositePass();
		void GTAOCompute();
		void BloomCompute();
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

		struct DrawSelectCommand
		{
			std::shared_ptr<Mesh> MeshInstance;
			std::shared_ptr<VertexBuffer> TransformBuffer;
			uint32_t SubmeshIndex;
			uint32_t InstanceCount;
		};

		struct MeshTransform
		{
			MeshTransform() { Transforms.reserve(10); }

			std::vector<TransformData> Transforms;
			std::shared_ptr<VertexBuffer> TransformBuffer = VertexBuffer::Create(10 * sizeof(TransformData));
		};

		struct MeshSelectTransform
		{
			MeshSelectTransform() { Transforms.reserve(10); }

			std::vector<SelectTransformData> Transforms;
			std::shared_ptr<VertexBuffer> TransformBuffer = VertexBuffer::Create(10 * sizeof(SelectTransformData));
		};

		struct CascadeMapData
		{
			std::array<float, SHADOW_MAP_CASCADE_COUNT> CascadeSplitLevels;
			glm::mat4 CascadeLightMatrices[SHADOW_MAP_CASCADE_COUNT];
		} m_CascadeData;

		struct LodAndMode
		{
			float LOD = 1.0f;
			float Mode = 1.0f; // 0->PreFilter, 1->Downsample, 2->Upsample-First, 3->Upsample
		} m_LodAndMode;

		struct SceneSettings
		{
			float Exposure = 1.0f;
			float DirtIntensity = 5.0f;
			uint32_t Fog = 0;
			float FogStartDistance = 5.5f;
			float FogFallOffDistance = 30.0f;
		} m_SceneSettings;

		struct BloomParams
		{
			float Threshold = 1.0f;
			float Knee = 0.5f;
		} m_BloomParams;

		struct SkyboxSettings
		{
			float Intensity = 0.05f;
			float LOD = 0.0f;
		} m_SkyboxSettings;

		struct ShadowSettings
		{
			glm::vec3 CascadeOrigin{ 0.0f };
			float CascadeSplitLambda = 0.92f;
			float CascadeNearPlaneOffset = 0.5f;
			float CascadeFarPlaneOffset = 48.0f;
			uint32_t MapSize = 4096;
			int CascadeOffset = 0;
		} m_CSMSettings;
	private:
		std::shared_ptr<Scene> m_Scene;

		std::shared_ptr<RenderCommandBuffer> m_SceneCommandBuffer;
		std::shared_ptr<Framebuffer> m_SceneFramebuffer;
		std::vector<VkDescriptorSet> m_SceneImages;
		std::vector<VkDescriptorSet> m_ShadowDepthPassImages;
		std::shared_ptr<PhysicsDebugRenderer> m_PhysicsDebugRenderer;

		// Pipelines
		std::shared_ptr<Pipeline> m_GeometryPipeline;
		std::shared_ptr<Pipeline> m_GeometryTessellatedPipeline;
		std::shared_ptr<Pipeline> m_GeometrySelectPipeline;
		std::shared_ptr<Pipeline> m_LinesPipeline;
		std::shared_ptr<Pipeline> m_ShadowMapPipeline;
		std::shared_ptr<Pipeline> m_LightPipeline;
		std::shared_ptr<Pipeline> m_LightSelectPipeline;
		std::shared_ptr<Pipeline> m_CompositePipeline;
		std::shared_ptr<Pipeline> m_SkyboxPipeline;
		std::shared_ptr<ComputePipeline> m_BloomPipeline;
		std::shared_ptr<ComputePipeline> m_GTAOPipeline; // Ground Truth Ambient Occlusion

		// Material Resources
		// Material per Shader set
		std::shared_ptr<Material> m_GeometryMaterial;
		std::shared_ptr<Material> m_GeometrySelectMaterial;
		std::shared_ptr<Material> m_LinesMaterial;
		std::shared_ptr<Material> m_ShadowMapMaterial;
		std::shared_ptr<Material> m_PointLightShaderMaterial;
		std::shared_ptr<Material> m_SpotLightShaderMaterial;
		std::shared_ptr<Material> m_LightSelectMaterial;
		std::shared_ptr<Material> m_CompositeShaderMaterial;
		std::shared_ptr<Material> m_SkyboxMaterial;
		std::shared_ptr<Material> m_GTAOMaterial;

		// Bloom Materials
		struct BloomComputeMaterials
		{
			std::shared_ptr<Material> PrefilterMaterial;
			std::vector<std::shared_ptr<Material>> PingMaterials;
			std::vector<std::shared_ptr<Material>> PongMaterials;
			std::shared_ptr<Material> FirstUpsampleMaterial;
			std::vector<std::shared_ptr<Material>> UpsampleMaterials;
		} m_BloomComputeMaterials;

		std::vector<std::shared_ptr<UniformBuffer>> m_UBCamera;
		std::vector<std::shared_ptr<UniformBuffer>> m_UBPointLight;
		std::vector<std::shared_ptr<UniformBuffer>> m_UBSpotLight;
		std::vector<std::shared_ptr<UniformBuffer>> m_UBDirectionalLight;
		std::vector<std::shared_ptr<UniformBuffer>> m_UBCascadeLightMatrices;
		std::vector<std::shared_ptr<IndexBuffer>> m_ImageBuffer; // For Selecting Entities

		std::vector<glm::vec4> m_PointLightPositions, m_SpotLightPositions;
		std::vector<uint32_t> m_LightHandles;

		std::vector<std::shared_ptr<Image2D>> m_BloomTextures;
		std::vector<std::shared_ptr<Image2D>> m_AOTextures;
		std::vector<std::shared_ptr<Image2D>> m_SceneRenderTextures; // For Bloom
#if VK_FEATURE_GTAO
		std::vector<std::shared_ptr<Image2D>> m_SceneDepthTextures; // For AO
#endif

		std::shared_ptr<Texture2D> m_BloomDirtTexture;
		std::shared_ptr<Texture2D> m_PointLightTextureIcon, m_SpotLightTextureIcon;

		// Skybox Resources
		std::shared_ptr<TextureCube> m_CubemapTexture, m_IrradianceTexture, m_PrefilteredTexture;
		std::shared_ptr<Image2D> m_BRDFTexture;
		VkDescriptorSet m_SkyboxTextureID;

		std::map<MeshKey, DrawCommand> m_MeshDrawList;
		std::map<MeshKey, MeshTransform> m_MeshTransformMap;
		std::map<MeshKey, DrawCommand> m_MeshTessellatedDrawList;
		std::map<MeshKey, MeshTransform> m_MeshTessellatedTransformMap;
		std::map<MeshKey, DrawCommand> m_ShadowMeshDrawList;
		std::map<MeshKey, MeshTransform> m_ShadowMeshTransformMap;
		std::map<MeshKey, DrawSelectCommand> m_SelectedMeshDrawList;
		std::map<MeshKey, MeshSelectTransform> m_SelectedMeshTransformMap;
		std::map<MeshKey, DrawCommand> m_PhysicsDebugMeshDrawList;
		std::map<MeshKey, MeshTransform> m_PhysicsDebugMeshTransformMap;

		glm::ivec2 m_ViewportSize = { 1920, 1080 };
		glm::uvec2 m_BloomMipSize;
		int m_HoveredEntity;

		SceneEditorData m_SceneEditorData;

		// TODO: Could be multiple instances but for now only one is required
		static SceneRenderer* s_Instance;
	};

}

namespace std {

	template<>
	struct hash<VulkanCore::SceneRenderer::MeshKey>
	{
		size_t operator()(const VulkanCore::SceneRenderer::MeshKey& other)
		{
			std::hash<uint64_t> h{};
			size_t hashVal = 0;
			hashVal ^= h(other.MeshHandle) + 0x9e3779b9 + (hashVal << 6) + (hashVal >> 2);
			hashVal ^= h(other.SubmeshIndex) + 0x9e3779b9 + (hashVal << 6) + (hashVal >> 2);

			return hashVal;
		}
	};

}
