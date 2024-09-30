#pragma once
#include "VulkanCore/Renderer/EditorCamera.h"
#include "VulkanCore/Renderer/ComputePipeline.h"
#include "VulkanCore/Renderer/UniformBuffer.h"
#include "VulkanCore/Renderer/StorageBuffer.h"
#include "VulkanCore/Renderer/VertexBuffer.h"
#include "VulkanCore/Renderer/RenderCommandBuffer.h"
#include "VulkanCore/Renderer/AccelerationStructure.h"
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
		void RasterizeScene();
		void TraceScene();
		void RenderLights();
		void SelectionPass();
		void SubmitMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const glm::mat4& transform);
		void SubmitSelectedMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const glm::mat4& transform, uint32_t entityID);
		void SubmitTransparentMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const glm::mat4& transform);
		void SubmitRayTracedMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const glm::mat4& transform);
		void UpdateMeshInstanceData(std::shared_ptr<Mesh> mesh, std::shared_ptr<MaterialAsset> materialAsset);
		void UpdateSkybox(const std::string& filepath);
		void SetUpdateTLAS();
		void SetRebuildAS();
		void ResetAccumulationFrameIndex();

		static SceneRenderer* GetSceneRenderer() { return s_Instance; }
		static inline VkDescriptorSet GetTextureCubeID() { return s_Instance->m_SkyboxTextureID; }
		static std::shared_ptr<RenderCommandBuffer> GetRenderCommandBuffer() { return s_Instance->m_SceneCommandBuffer; }
		static void SetSkybox(const std::string& filepath);
		void SetSceneEditorData(const SceneEditorData& sceneEditorData) { m_SceneEditorData = sceneEditorData; }

		bool IsRayTraced() const;
		inline int GetHoveredEntity() const { return m_HoveredEntity; }
		inline glm::ivec2 GetViewportSize() const { return m_ViewportSize; }
		std::shared_ptr<Image2D> GetFinalPassImage(uint32_t index) const;
		inline VkDescriptorSet GetSceneImage(uint32_t index) const { return m_SceneImages[index]; }
	private:
		void CreateCommandBuffers();
		void CreatePipelines();
		void CreateResources();
		void CreateMaterials();
		void SetRayTraceMaterialsData();
		void UpdateAccelerationStructures();
		void UpdateRayTraceResources();
		void RecreateMaterials();
		void RecreatePipelines();

		void GeometryPass();
		void CompositePass();
		void BloomCompute();
		void RayTracePass();
		void ResetDrawCommands();
		void ResetTraceCommands();
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
		
		struct DrawSelectCommand
		{
			std::shared_ptr<Mesh> MeshInstance;
			std::shared_ptr<VertexBuffer> TransformBuffer;
			uint32_t SubmeshIndex;
			uint32_t InstanceCount;
		};

		struct MeshBuffersAddress
		{
			uint64_t VertexBufferAddress = 0;
			uint64_t IndexBufferAddress = 0;
		};

		struct MeshTransform
		{
			MeshTransform()
			{
				Transforms.reserve(10);
			}

			std::vector<TransformData> Transforms;
			std::shared_ptr<VertexBuffer> TransformBuffer = VertexBuffer::Create(10 * sizeof(TransformData));
		};

		struct MeshSelectTransform
		{
			MeshSelectTransform() = default;

			std::vector<SelectTransformData> Transforms;
			std::shared_ptr<VertexBuffer> TransformBuffer = VertexBuffer::Create(10 * sizeof(SelectTransformData));
		};

		struct LodAndMode
		{
			float LOD = 1.0f;
			float Mode = 1.0f; // 0->PreFilter, 1->Downsample, 2->Upsample-First, 3->Upsample
		} m_LodAndMode;

		struct SceneSettings
		{
			float Exposure = 1.0f;
			float DirtIntensity = 0.0f;
			uint32_t Fog = 0;
		} m_SceneSettings;

		struct BloomParams
		{
			float Threshold = 1.0f;
			float Knee = 0.5f;
		} m_BloomParams;

		struct SkyboxSettings
		{
			float Intensity = 0.25f;
			float LOD = 0.0f;
		} m_SkyboxSettings;

		struct RTSettings
		{
			uint32_t SampleCount = 4;
			uint32_t Bounces = 4;
			uint32_t RandomSeed = 1;
			uint32_t AccumulateFrameIndex = 1;
		} m_RTSettings;

		struct RTMaterialData
		{
			glm::vec4 Extinction_AtDistance = glm::vec4{ 0.75f, 0.75f, 0.75f, 0.1f }; // Extinction => XYZ, At Distance => W
			float Transmission = 0.25f;
			float SpecularTint = 0.1f;
			float IOR = 1.5f;
			float Sheen = 0.75f;
			float SheenTint = 0.5f;
			float Clearcoat = 0.25f;
			float ClearcoatGloss = 0.8f;
			float Anisotropy = 0.5f;
			float Subsurface = 0.8f;
		} m_RTMaterialData;
	private:
		std::shared_ptr<Scene> m_Scene;

		std::shared_ptr<RenderCommandBuffer> m_SceneCommandBuffer;
		std::shared_ptr<AccelerationStructure> m_SceneAccelerationStructure;
		std::shared_ptr<Framebuffer> m_SceneFramebuffer;
		std::vector<VkDescriptorSet> m_SceneImages;

		// Pipelines
		std::shared_ptr<Pipeline> m_GeometryPipeline;
		std::shared_ptr<Pipeline> m_GeometrySelectPipeline;
		std::shared_ptr<Pipeline> m_LightPipeline;
		std::shared_ptr<Pipeline> m_LightSelectPipeline;
		std::shared_ptr<Pipeline> m_CompositePipeline;
		std::shared_ptr<Pipeline> m_SkyboxPipeline;
		std::shared_ptr<ComputePipeline> m_BloomPipeline;

		// Ray Tracing
		std::shared_ptr<RayTracingPipeline> m_RayTracingPipeline;
		std::shared_ptr<ShaderBindingTable> m_RayTracingSBT;

		// TODO: In future we have to setup Material Table
		// Material Resources
		// Material per Shader set
		std::shared_ptr<Material> m_GeometryMaterial;
		std::shared_ptr<Material> m_GeometrySelectMaterial;
		std::shared_ptr<Material> m_PointLightShaderMaterial;
		std::shared_ptr<Material> m_SpotLightShaderMaterial;
		std::shared_ptr<Material> m_LightSelectMaterial;
		std::shared_ptr<Material> m_CompositeShaderMaterial;
		std::shared_ptr<Material> m_SkyboxMaterial;

		// Bloom Materials
		std::shared_ptr<Material> m_BloomPrefilterShaderMaterial;
		std::vector<std::shared_ptr<Material>> m_BloomPingShaderMaterials;
		std::vector<std::shared_ptr<Material>> m_BloomPongShaderMaterials;
		std::shared_ptr<Material> m_BloomUpsampleFirstShaderMaterial;
		std::vector<std::shared_ptr<Material>> m_BloomUpsampleShaderMaterials;

		// Ray Tracing Materials
		std::shared_ptr<Material> m_RayTracingBaseMaterial;
		std::shared_ptr<Material> m_RayTracingPBRMaterial;
		std::shared_ptr<Material> m_RayTracingSkyboxMaterial;

		// Resources
		std::vector<std::shared_ptr<UniformBuffer>> m_UBCamera;
		std::vector<std::shared_ptr<UniformBuffer>> m_UBPointLight;
		std::vector<std::shared_ptr<UniformBuffer>> m_UBSpotLight;
		std::vector<std::shared_ptr<UniformBuffer>> m_UBSkyboxSettings;
		std::vector<std::shared_ptr<UniformBuffer>> m_UBRTMaterialData;
		std::vector<std::shared_ptr<IndexBuffer>> m_ImageBuffer;

		std::vector<std::shared_ptr<StorageBuffer>> m_SBMeshBuffersData;
		std::vector<std::shared_ptr<StorageBuffer>> m_SBMaterialDataBuffer;

		std::vector<glm::vec4> m_PointLightPositions, m_SpotLightPositions;
		std::vector<uint32_t> m_LightHandles;

		std::vector<std::shared_ptr<Image2D>> m_BloomTextures;
		std::vector<std::shared_ptr<Image2D>> m_SceneRenderTextures;
		std::vector<std::shared_ptr<Image2D>> m_SceneRTOutputImages;
		std::vector<std::shared_ptr<Image2D>> m_SceneRTAccumulationImages;

		std::shared_ptr<Texture2D> m_BloomDirtTexture;
		std::shared_ptr<Texture2D> m_PointLightTextureIcon, m_SpotLightTextureIcon;

		// Ray Tracing Resources
		std::vector<std::shared_ptr<Texture2D>> m_DiffuseTextureArray;
		std::vector<std::shared_ptr<Texture2D>> m_NormalTextureArray;
		std::vector<std::shared_ptr<Texture2D>> m_ARMTextureArray;

		// Skybox Resources
		std::shared_ptr<TextureCube> m_PrefilteredTexture, m_IrradianceTexture;
		std::shared_ptr<Texture2D> m_HDRTexture, m_PDFTexture, m_CDFTexture;
		std::shared_ptr<Image2D> m_BRDFTexture;
		VkDescriptorSet m_SkyboxTextureID;

		std::map<MeshKey, DrawCommand> m_MeshDrawList;
		std::map<MeshKey, TraceCommand> m_MeshTraceList;
		std::map<MeshKey, MeshTransform> m_MeshTransformMap;
		std::map<MeshKey, DrawSelectCommand> m_SelectedMeshDrawList;
		std::map<MeshKey, MeshSelectTransform> m_SelectedMeshTransformMap;

		glm::ivec2 m_ViewportSize = { 1920, 1080 };
		glm::uvec2 m_BloomMipSize;
		uint32_t m_MaxAccumulateFrameCount = 10000;
		bool m_Accumulate = false, m_UpdateTLAS = false, m_RebuildAS = false;
		int m_HoveredEntity;

		SceneEditorData m_SceneEditorData;

		// TODO: Could be multiple instances but for now only one is required
		static SceneRenderer* s_Instance;
	};

}
