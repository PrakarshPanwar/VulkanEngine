#include "vulkanpch.h"
#include "SceneRenderer.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Core/Timer.h"
#include "VulkanCore/Core/ImGuiLayer.h"
#include "VulkanCore/Events/Input.h"
#include "VulkanCore/Asset/AssetManager.h"
#include "VulkanCore/Asset/TextureImporter.h"
#include "VulkanCore/Asset/MaterialAsset.h"
#include "VulkanCore/Renderer/Renderer.h"

#include "VulkanCore/Mesh/Mesh.h"
#include "Platform/Vulkan/VulkanAllocator.h"
#include "Platform/Vulkan/VulkanMaterial.h"
#include "Platform/Vulkan/VulkanFramebuffer.h"
#include "Platform/Vulkan/VulkanRenderPass.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "Platform/Vulkan/VulkanComputePipeline.h"
#include "Platform/Vulkan/VulkanUniformBuffer.h"
#include "Platform/Vulkan/VulkanStorageBuffer.h"
#include "Platform/Vulkan/VulkanVertexBuffer.h"
#include "Platform/Vulkan/VulkanIndexBuffer.h"
#include "Platform/Vulkan/Utils/ImageUtils.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace VulkanCore {

	namespace Utils {

		static uint32_t CalculateMipCount(uint32_t width, uint32_t height)
		{
			return (uint32_t)std::_Floor_of_log_2(std::max(width, height)) + 1;
		}

	}

	SceneRenderer* SceneRenderer::s_Instance = nullptr;

	SceneRenderer::SceneRenderer(std::shared_ptr<Scene> scene)
		: m_Scene(scene)
	{
		s_Instance = this;
		Init();
	}

	SceneRenderer::~SceneRenderer()
	{
		Release();
	}

	void SceneRenderer::Init()
	{
		CreateCommandBuffers();
		CreatePipelines();
		CreateResources();
		CreateMaterials();
	}

	void SceneRenderer::CreatePipelines()
	{
		VertexBufferLayout vertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float3, "a_Tangent" },
			{ ShaderDataType::Float3, "a_Binormal" },
			{ ShaderDataType::Float2, "a_TexCoord" }
		};

		VertexBufferLayout instanceLayout = {
			{ ShaderDataType::Float4, "a_MRow0" },
			{ ShaderDataType::Float4, "a_MRow1" },
			{ ShaderDataType::Float4, "a_MRow2" }
		};

		// Geometry and Point Light Pipeline
		{
			FramebufferSpecification geomFramebufferSpec;
			geomFramebufferSpec.Width = 1920;
			geomFramebufferSpec.Height = 1080;
			geomFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::RGBA8_UNORM, ImageFormat::RGBA8_SRGB, ImageFormat::DEPTH32F };
			geomFramebufferSpec.ReadDepthTexture = true;
			geomFramebufferSpec.Transfer = true;
			geomFramebufferSpec.Samples = 4;

			RenderPassSpecification geomRenderPassSpec;
			geomRenderPassSpec.TargetFramebuffer = std::make_shared<VulkanFramebuffer>(geomFramebufferSpec);

			PipelineSpecification geomPipelineSpec;
			geomPipelineSpec.DebugName = "Geometry Pipeline";
			geomPipelineSpec.pShader = Renderer::GetShader("CorePBR");
			geomPipelineSpec.Blend = true;
			geomPipelineSpec.pRenderPass = std::make_shared<VulkanRenderPass>(geomRenderPassSpec);
			geomPipelineSpec.Layout = vertexLayout;
			geomPipelineSpec.InstanceLayout = instanceLayout;

			PipelineSpecification geomTessellatedPipelineSpec;
			geomTessellatedPipelineSpec.DebugName = "Geometry Tessellated Pipeline";
			geomTessellatedPipelineSpec.pShader = Renderer::GetShader("CorePBR_Tess");
			geomTessellatedPipelineSpec.Topology = PrimitiveTopology::PatchList;
			geomTessellatedPipelineSpec.PatchControlPoints = 3; // Triangles
			geomTessellatedPipelineSpec.pRenderPass = geomPipelineSpec.pRenderPass;
			geomTessellatedPipelineSpec.Layout = vertexLayout;
			geomTessellatedPipelineSpec.InstanceLayout = instanceLayout;

			PipelineSpecification lightPipelineSpec;
			lightPipelineSpec.DebugName = "Light Pipeline";
			lightPipelineSpec.pShader = Renderer::GetShader("LightShader");
			lightPipelineSpec.Blend = true;
			lightPipelineSpec.pRenderPass = geomPipelineSpec.pRenderPass;

			m_GeometryPipeline = std::make_shared<VulkanPipeline>(geomPipelineSpec);
			m_GeometryTessellatedPipeline = std::make_shared<VulkanPipeline>(geomTessellatedPipelineSpec);
			m_LightPipeline = std::make_shared<VulkanPipeline>(lightPipelineSpec);
		}

		// Geometry/Light Selection Pipeline(Editor Only)
		{
			FramebufferSpecification geomSelectFramebufferSpec;
			geomSelectFramebufferSpec.Width = 1920;
			geomSelectFramebufferSpec.Height = 1080;
			geomSelectFramebufferSpec.Attachments = { ImageFormat::R32I, ImageFormat::DEPTH24STENCIL8 };
			geomSelectFramebufferSpec.Transfer = true;
			geomSelectFramebufferSpec.Samples = 1;
			memset(&geomSelectFramebufferSpec.ClearColor, -1, sizeof(glm::vec4));

			RenderPassSpecification geomSelectRenderPassSpec;
			geomSelectRenderPassSpec.TargetFramebuffer = std::make_shared<VulkanFramebuffer>(geomSelectFramebufferSpec);

			PipelineSpecification geomSelectPipelineSpec;
			geomSelectPipelineSpec.DebugName = "Geometry Select Pipeline";
			geomSelectPipelineSpec.pShader = Renderer::GetShader("CoreEditor");
			geomSelectPipelineSpec.pRenderPass = std::make_shared<VulkanRenderPass>(geomSelectRenderPassSpec);
			geomSelectPipelineSpec.Layout = vertexLayout;
			geomSelectPipelineSpec.InstanceLayout = {
				{ ShaderDataType::Float4, "a_MRow0" },
				{ ShaderDataType::Float4, "a_MRow1" },
				{ ShaderDataType::Float4, "a_MRow2" },
				{ ShaderDataType::Int, "a_EntityID" }
			};

			m_GeometrySelectPipeline = std::make_shared<VulkanPipeline>(geomSelectPipelineSpec);

			PipelineSpecification lightSelectPipelineSpec;
			lightSelectPipelineSpec.DebugName = "Light Select Pipeline";
			lightSelectPipelineSpec.pShader = Renderer::GetShader("LightEditor");
			lightSelectPipelineSpec.pRenderPass = geomSelectPipelineSpec.pRenderPass;

			m_LightSelectPipeline = std::make_shared<VulkanPipeline>(lightSelectPipelineSpec);
		}

		// Lines Pipeline
		{
			PipelineSpecification linesPipelineSpec;
			linesPipelineSpec.DebugName = "Lines Pipeline";
			linesPipelineSpec.pShader = Renderer::GetShader("Lines");
			linesPipelineSpec.Topology = PrimitiveTopology::LineList;
			linesPipelineSpec.pRenderPass = m_GeometryPipeline->GetSpecification().pRenderPass;
			linesPipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float4, "a_Color" }
			};

			m_LinesPipeline = std::make_shared<VulkanPipeline>(linesPipelineSpec);
		}

		// Shadow Map Pipeline(NOTE: Currently not working properly)
		{
			FramebufferSpecification shadowMapFramebufferSpec;
			shadowMapFramebufferSpec.Width = m_CSMSettings.MapSize;
			shadowMapFramebufferSpec.Height = m_CSMSettings.MapSize;
			shadowMapFramebufferSpec.Attachments = { ImageFormat::DEPTH32F };
			shadowMapFramebufferSpec.ReadDepthTexture = true;
			shadowMapFramebufferSpec.Transfer = true;
			shadowMapFramebufferSpec.Samples = 1;
			shadowMapFramebufferSpec.Layers = SHADOW_MAP_CASCADE_COUNT;

			RenderPassSpecification shadowMapRenderPassSpec;
			shadowMapRenderPassSpec.TargetFramebuffer = std::make_shared<VulkanFramebuffer>(shadowMapFramebufferSpec);

			PipelineSpecification shadowMapPipelineSpec;
			shadowMapPipelineSpec.DebugName = "Shadow Map Pipeline";
			shadowMapPipelineSpec.pShader = Renderer::GetShader("ShadowDepth");
			shadowMapPipelineSpec.pRenderPass = std::make_shared<VulkanRenderPass>(shadowMapRenderPassSpec);
			shadowMapPipelineSpec.DepthClamp = true;
			shadowMapPipelineSpec.Layout = vertexLayout;
			shadowMapPipelineSpec.InstanceLayout = instanceLayout;
			shadowMapPipelineSpec.DepthCompareOp = CompareOp::LessOrEqual;

			m_ShadowMapPipeline = std::make_shared<VulkanPipeline>(shadowMapPipelineSpec);
		}

		// Composite Pipeline
		{
			FramebufferSpecification compFramebufferSpec;
			compFramebufferSpec.Width = 1920;
			compFramebufferSpec.Height = 1080;
			compFramebufferSpec.Attachments = { ImageFormat::RGBA8_SRGB };
			compFramebufferSpec.Samples = 1;

			RenderPassSpecification compRenderPassSpec;
			compRenderPassSpec.TargetFramebuffer = std::make_shared<VulkanFramebuffer>(compFramebufferSpec);
			m_SceneFramebuffer = compRenderPassSpec.TargetFramebuffer;

			PipelineSpecification compPipelineSpec;
			compPipelineSpec.DebugName = "Composite Pipeline";
			compPipelineSpec.pShader = Renderer::GetShader("SceneComposite");
			compPipelineSpec.pRenderPass = std::make_shared<VulkanRenderPass>(compRenderPassSpec);
			compPipelineSpec.DepthTest = false;
			compPipelineSpec.DepthWrite = false;

			m_CompositePipeline = std::make_shared<VulkanPipeline>(compPipelineSpec);
		}

		// Skybox Pipeline
		{
			PipelineSpecification skyboxPipelineSpec;
			skyboxPipelineSpec.DebugName = "Skybox Pipeline";
			skyboxPipelineSpec.pShader = Renderer::GetShader("Skybox");
			skyboxPipelineSpec.pRenderPass = m_GeometryPipeline->GetSpecification().pRenderPass;
			skyboxPipelineSpec.DepthCompareOp = CompareOp::LessOrEqual;

			m_SkyboxPipeline = std::make_shared<VulkanPipeline>(skyboxPipelineSpec);
		}

		// Bloom Pipeline
		{
			m_BloomPipeline = std::make_shared<VulkanComputePipeline>(Renderer::GetShader("Bloom"), "Bloom Pipeline");
		}

		// Ground Truth Ambient Occlusion
		{
			m_GTAOPipeline = std::make_shared<VulkanComputePipeline>(Renderer::GetShader("GTAO"), "GTAO");
		}
	}

	void SceneRenderer::CreateMaterials()
	{
		// Geometry Material
		{
			m_GeometryMaterial = std::make_shared<VulkanMaterial>(m_GeometryPipeline->GetSpecification().pShader, "Geometry Shader Material");

			m_GeometryMaterial->SetBuffers(0, m_UBCamera);
			m_GeometryMaterial->SetBuffers(1, m_UBPointLight);
			m_GeometryMaterial->SetBuffers(2, m_UBSpotLight);
			m_GeometryMaterial->SetBuffers(3, m_UBDirectionalLight);
			m_GeometryMaterial->SetBuffers(4, m_UBCascadeLightMatrices);
			m_GeometryMaterial->SetTexture(6, m_IrradianceTexture);
			m_GeometryMaterial->SetImage(7, m_BRDFTexture);
			m_GeometryMaterial->SetTexture(8, m_PrefilteredTexture);
			m_GeometryMaterial->SetImages(9, m_ShadowMapPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer->GetDepthAttachment());
			m_GeometryMaterial->PrepareShaderMaterial();
		}

		// Geometry Select Material
		{
			m_GeometrySelectMaterial = std::make_shared<VulkanMaterial>(m_GeometrySelectPipeline->GetSpecification().pShader, "Geometry Select Shader Material");

			m_GeometrySelectMaterial->SetBuffers(0, m_UBCamera);
			m_GeometrySelectMaterial->PrepareShaderMaterial();
		}

		// Lines Material
		{
			m_LinesMaterial = std::make_shared<VulkanMaterial>(m_LinesPipeline->GetSpecification().pShader, "Lines Material");

			m_LinesMaterial->SetBuffers(0, m_UBCamera);
			m_LinesMaterial->PrepareShaderMaterial();
		}

		// Light Materials
		{
			m_PointLightShaderMaterial = std::make_shared<VulkanMaterial>(m_LightPipeline->GetSpecification().pShader, "Point Light Shader Material");
			m_SpotLightShaderMaterial = std::make_shared<VulkanMaterial>(m_LightPipeline->GetSpecification().pShader, "Spot Light Shader Material");

			m_PointLightShaderMaterial->SetBuffers(0, m_UBCamera);
			m_PointLightShaderMaterial->SetTexture(1, std::dynamic_pointer_cast<VulkanTexture>(m_PointLightTextureIcon));
			m_PointLightShaderMaterial->PrepareShaderMaterial();

			m_SpotLightShaderMaterial->SetBuffers(0, m_UBCamera);
			m_SpotLightShaderMaterial->SetTexture(1, std::dynamic_pointer_cast<VulkanTexture>(m_SpotLightTextureIcon));
			m_SpotLightShaderMaterial->PrepareShaderMaterial();

			m_LightSelectMaterial = std::make_shared<VulkanMaterial>(m_LightSelectPipeline->GetSpecification().pShader, "Light Select Shader Material");

			m_LightSelectMaterial->SetBuffers(0, m_UBCamera);
			m_LightSelectMaterial->PrepareShaderMaterial();
		}

		// Shadow Map Material
		{
			m_ShadowMapMaterial = std::make_shared<VulkanMaterial>(m_ShadowMapPipeline->GetSpecification().pShader, "Shadow Map Material");

			m_ShadowMapMaterial->SetBuffers(1, m_UBCascadeLightMatrices);
			m_ShadowMapMaterial->PrepareShaderMaterial();
		}

		// Bloom Materials
		{
			const uint32_t mipCount = m_BloomTextures[0]->GetSpecification().MipLevels;
			auto bloomTexture0 = std::static_pointer_cast<VulkanImage>(m_BloomTextures[0]);
			auto bloomTexture1 = std::static_pointer_cast<VulkanImage>(m_BloomTextures[1]);
			auto bloomTexture2 = std::static_pointer_cast<VulkanImage>(m_BloomTextures[2]);

			// Set A: Prefiltering
			// Binding 0(o_Image): BloomTex[0]
			// Binding 1(u_Texture): RenderTex
			// Binding 2(u_BloomTexture): RenderTex
			m_BloomComputeMaterials.PrefilterMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Prefilter Shader Material");

			m_BloomComputeMaterials.PrefilterMaterial->SetImage(0, m_BloomTextures[0]);
			m_BloomComputeMaterials.PrefilterMaterial->SetImages(1, m_SceneRenderTextures);
			m_BloomComputeMaterials.PrefilterMaterial->SetImages(2, m_SceneRenderTextures);
			m_BloomComputeMaterials.PrefilterMaterial->PrepareShaderMaterial();

			m_BloomComputeMaterials.PingMaterials.resize(mipCount - 1);
			m_BloomComputeMaterials.PongMaterials.resize(mipCount - 1);
			for (uint32_t i = 1; i < mipCount; ++i)
			{
				// Set B: Downsampling(Ping)
				// Binding 0(o_Image): BloomTex[1]
				// Binding 1(u_Texture): BloomTex[0]
				// Binding 2(u_BloomTexture): RenderTex
				auto bloomPingShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Ping Shader Material");

				bloomTexture1->CreateImageViewSingleMip(i);
				bloomPingShaderMaterial->SetImage(0, m_BloomTextures[1], i);
				bloomPingShaderMaterial->SetImage(1, m_BloomTextures[0]);
				bloomPingShaderMaterial->SetImages(2, m_SceneRenderTextures);
				bloomPingShaderMaterial->PrepareShaderMaterial();

				m_BloomComputeMaterials.PingMaterials[i - 1] = bloomPingShaderMaterial;

				// Set C: Downsampling(Pong)
				// Binding 0(o_Image): BloomTex[0]
				// Binding 1(u_Texture): BloomTex[1]
				// Binding 2(u_BloomTexture): RenderTex
				auto bloomPongShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Pong Shader Material");

				bloomTexture0->CreateImageViewSingleMip(i);
				bloomPongShaderMaterial->SetImage(0, m_BloomTextures[0], i);
				bloomPongShaderMaterial->SetImage(1, m_BloomTextures[1]);
				bloomPongShaderMaterial->SetImages(2, m_SceneRenderTextures);
				bloomPongShaderMaterial->PrepareShaderMaterial();

				m_BloomComputeMaterials.PongMaterials[i - 1] = bloomPongShaderMaterial;
			}

			// Set D: First Upsampling
			// Binding 0(o_Image): BloomTex[2]
			// Binding 1(u_Texture): BloomTex[0]
			// Binding 2(u_BloomTexture): RenderTex
			m_BloomComputeMaterials.FirstUpsampleMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Upsample First Shader Material");

			bloomTexture2->CreateImageViewSingleMip(mipCount - 1);
			m_BloomComputeMaterials.FirstUpsampleMaterial->SetImage(0, m_BloomTextures[2], mipCount - 1);
			m_BloomComputeMaterials.FirstUpsampleMaterial->SetImage(1, m_BloomTextures[0]);
			m_BloomComputeMaterials.FirstUpsampleMaterial->SetImages(2, m_SceneRenderTextures);
			m_BloomComputeMaterials.FirstUpsampleMaterial->PrepareShaderMaterial();

			// Set E: Final Upsampling
			// Binding 0(o_Image): BloomTex[2]
			// Binding 1(u_Texture): BloomTex[0]
			// Binding 2(u_BloomTexture): BloomTex[2]
			m_BloomComputeMaterials.UpsampleMaterials.resize(mipCount - 1);
			for (int i = mipCount - 2; i >= 0; --i)
			{
				auto bloomUpsampleShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Upsample Shader Material");

				bloomTexture2->CreateImageViewSingleMip(i);
				bloomUpsampleShaderMaterial->SetImage(0, m_BloomTextures[2], i);
				bloomUpsampleShaderMaterial->SetImage(1, m_BloomTextures[0]);
				bloomUpsampleShaderMaterial->SetImage(2, m_BloomTextures[2]);
				bloomUpsampleShaderMaterial->PrepareShaderMaterial();

				m_BloomComputeMaterials.UpsampleMaterials[i] = bloomUpsampleShaderMaterial;
			}
		}

		// AO Material
		{
			m_GTAOMaterial = std::make_shared<VulkanMaterial>(m_GTAOPipeline->GetShader(), "GTAO Material");

			auto geomFB = std::dynamic_pointer_cast<VulkanFramebuffer>(m_GeometryPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer);
			m_GTAOMaterial->SetImages(0, m_AOTextures);
			m_GTAOMaterial->SetBuffers(1, m_UBCamera);
			m_GTAOMaterial->SetImages(2, geomFB->GetDepthAttachment());
			m_GTAOMaterial->SetImages(3, geomFB->GetAttachment(1));
			m_GTAOMaterial->SetImages(4, geomFB->GetAttachment(2));
			m_GTAOMaterial->PrepareShaderMaterial();
		}

		// Composite Material
		{
			m_CompositeShaderMaterial = std::make_shared<VulkanMaterial>(m_CompositePipeline->GetSpecification().pShader, "Composite Shader Material");

			auto geomFB = std::dynamic_pointer_cast<VulkanFramebuffer>(m_GeometryPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer);
			m_CompositeShaderMaterial->SetBuffers(0, m_UBCamera);
			m_CompositeShaderMaterial->SetImages(1, geomFB->GetAttachment());
			m_CompositeShaderMaterial->SetImages(2, geomFB->GetDepthAttachment());
			m_CompositeShaderMaterial->SetImage(3, m_BloomTextures[2]);
			m_CompositeShaderMaterial->SetTexture(4, std::dynamic_pointer_cast<VulkanTexture>(m_BloomDirtTexture));
			m_CompositeShaderMaterial->PrepareShaderMaterial();
		}

		// Skybox Material
		{
			m_SkyboxMaterial = std::make_shared<VulkanMaterial>(m_SkyboxPipeline->GetSpecification().pShader, "Skybox Shader Material");

			m_SkyboxMaterial->SetBuffers(0, m_UBCamera);
			m_SkyboxMaterial->SetTexture(1, m_CubemapTexture);
			m_SkyboxMaterial->PrepareShaderMaterial();
		}
	}

	void SceneRenderer::RecreateMaterials()
	{
		// Geometry Material
		{
			m_GeometryMaterial->SetBuffers(4, m_UBCascadeLightMatrices);
			m_GeometryMaterial->SetImages(9, m_ShadowMapPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer->GetDepthAttachment());
			m_GeometryMaterial->PrepareShaderMaterial();
		}

		// Shadow Map Material
		{
			m_ShadowMapMaterial->SetBuffers(1, m_UBCascadeLightMatrices);
			m_ShadowMapMaterial->PrepareShaderMaterial();
		}

		// Bloom Materials
		{
			const uint32_t mipCount = m_BloomTextures[0]->GetSpecification().MipLevels;
			auto bloomTexture0 = std::static_pointer_cast<VulkanImage>(m_BloomTextures[0]);
			auto bloomTexture1 = std::static_pointer_cast<VulkanImage>(m_BloomTextures[1]);
			auto bloomTexture2 = std::static_pointer_cast<VulkanImage>(m_BloomTextures[2]);

			// Set A: Prefiltering
			// Binding 0(o_Image): BloomTex[0]
			// Binding 1(u_Texture): RenderTex
			// Binding 2(u_BloomTexture): RenderTex
			m_BloomComputeMaterials.PrefilterMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Prefilter Shader Material");

			m_BloomComputeMaterials.PrefilterMaterial->SetImage(0, m_BloomTextures[0]);
			m_BloomComputeMaterials.PrefilterMaterial->SetImages(1, m_SceneRenderTextures);
			m_BloomComputeMaterials.PrefilterMaterial->SetImages(2, m_SceneRenderTextures);
			m_BloomComputeMaterials.PrefilterMaterial->PrepareShaderMaterial();

			m_BloomComputeMaterials.PingMaterials.resize(mipCount - 1);
			m_BloomComputeMaterials.PongMaterials.resize(mipCount - 1);
			for (uint32_t i = 1; i < mipCount; ++i)
			{
				// Set B: Downsampling(Ping)
				// Binding 0(o_Image): BloomTex[1]
				// Binding 1(u_Texture): BloomTex[0]
				// Binding 2(u_BloomTexture): RenderTex
				auto bloomPingShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Ping Shader Material");

				bloomTexture1->CreateImageViewSingleMip(i);
				bloomPingShaderMaterial->SetImage(0, m_BloomTextures[1], i);
				bloomPingShaderMaterial->SetImage(1, m_BloomTextures[0]);
				bloomPingShaderMaterial->SetImages(2, m_SceneRenderTextures);
				bloomPingShaderMaterial->PrepareShaderMaterial();

				m_BloomComputeMaterials.PingMaterials[i - 1] = bloomPingShaderMaterial;

				// Set C: Downsampling(Pong)
				// Binding 0(o_Image): BloomTex[0]
				// Binding 1(u_Texture): BloomTex[1]
				// Binding 2(u_BloomTexture): RenderTex
				auto bloomPongShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Pong Shader Material");

				bloomTexture0->CreateImageViewSingleMip(i);
				bloomPongShaderMaterial->SetImage(0, m_BloomTextures[0], i);
				bloomPongShaderMaterial->SetImage(1, m_BloomTextures[1]);
				bloomPongShaderMaterial->SetImages(2, m_SceneRenderTextures);
				bloomPongShaderMaterial->PrepareShaderMaterial();

				m_BloomComputeMaterials.PongMaterials[i - 1] = bloomPongShaderMaterial;
			}

			// Set D: First Upsampling
			// Binding 0(o_Image): BloomTex[2]
			// Binding 1(u_Texture): BloomTex[0]
			// Binding 2(u_BloomTexture): RenderTex
			m_BloomComputeMaterials.FirstUpsampleMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Upsample First Shader Material");

			bloomTexture2->CreateImageViewSingleMip(mipCount - 1);
			m_BloomComputeMaterials.FirstUpsampleMaterial->SetImage(0, m_BloomTextures[2], mipCount - 1);
			m_BloomComputeMaterials.FirstUpsampleMaterial->SetImage(1, m_BloomTextures[0]);
			m_BloomComputeMaterials.FirstUpsampleMaterial->SetImages(2, m_SceneRenderTextures);
			m_BloomComputeMaterials.FirstUpsampleMaterial->PrepareShaderMaterial();

			// Set E: Final Upsampling
			// Binding 0(o_Image): BloomTex[2]
			// Binding 1(u_Texture): BloomTex[0]
			// Binding 2(u_BloomTexture): BloomTex[2]
			m_BloomComputeMaterials.UpsampleMaterials.resize(mipCount - 1);
			for (int i = mipCount - 2; i >= 0; --i)
			{
				auto bloomUpsampleShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Upsample Shader Material");

				bloomTexture2->CreateImageViewSingleMip(i);
				bloomUpsampleShaderMaterial->SetImage(0, m_BloomTextures[2], i);
				bloomUpsampleShaderMaterial->SetImage(1, m_BloomTextures[0]);
				bloomUpsampleShaderMaterial->SetImage(2, m_BloomTextures[2]);
				bloomUpsampleShaderMaterial->PrepareShaderMaterial();

				m_BloomComputeMaterials.UpsampleMaterials[i] = bloomUpsampleShaderMaterial;
			}
		}

		// Composite Material
		{
			auto geomFB = std::dynamic_pointer_cast<VulkanFramebuffer>(m_GeometryPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer);

			m_CompositeShaderMaterial->SetBuffers(0, m_UBCamera);
			m_CompositeShaderMaterial->SetImages(1, geomFB->GetAttachment());
			m_CompositeShaderMaterial->SetImages(2, geomFB->GetDepthAttachment());
			m_CompositeShaderMaterial->SetImage(3, m_BloomTextures[2]);
			m_CompositeShaderMaterial->SetTexture(4, std::dynamic_pointer_cast<VulkanTexture>(m_BloomDirtTexture));
			m_CompositeShaderMaterial->PrepareShaderMaterial();
		}

		// AO Material
		{
			auto geomFB = std::dynamic_pointer_cast<VulkanFramebuffer>(m_GeometryPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer);

			m_GTAOMaterial->SetImages(0, m_AOTextures);
			m_GTAOMaterial->SetBuffers(1, m_UBCamera);
			m_GTAOMaterial->SetImages(2, geomFB->GetDepthAttachment());
			m_GTAOMaterial->SetImages(3, geomFB->GetAttachment(1));
			m_GTAOMaterial->SetImages(4, geomFB->GetAttachment(2));
			m_GTAOMaterial->PrepareShaderMaterial();
		}

		// Skybox Material
		{
			m_SkyboxMaterial->SetBuffers(0, m_UBCamera);
			m_SkyboxMaterial->SetTexture(1, m_CubemapTexture);
			m_SkyboxMaterial->PrepareShaderMaterial();
		}
	}

	void SceneRenderer::RecreatePipelines()
	{
		m_ShadowMapPipeline->ReloadPipeline();
		m_GeometryPipeline->ReloadPipeline();
		m_GeometryTessellatedPipeline->ReloadPipeline();
		m_LightPipeline->ReloadPipeline();
		m_CompositePipeline->ReloadPipeline();
		m_SkyboxPipeline->ReloadPipeline();
		m_BloomPipeline->ReloadPipeline();
	}

	std::vector<glm::vec4> SceneRenderer::GetFrustumCornersWorldSpace()
	{
		const auto projViewInv = glm::inverse(m_SceneEditorData.CameraData.GetProjectionMatrix() * m_SceneEditorData.CameraData.GetViewMatrix());

		std::vector<glm::vec4> corners{};
		for (uint32_t x = 0; x < 2; ++x)
		{
			for (uint32_t y = 0; y < 2; ++y)
			{
				for (uint32_t z = 0; z < 2; ++z)
				{
					const glm::vec4 pt = projViewInv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
					corners.push_back(pt / pt.w);
				}
			}
		}

		return corners;
	}

	void SceneRenderer::UpdateCascadeMap()
	{
		std::array<float, SHADOW_MAP_CASCADE_COUNT> cascadeSplits{};

		const auto& cameraData = m_SceneEditorData.CameraData;
		glm::vec2 nearFarClip = cameraData.GetNearFarClip();
		float clipRange = nearFarClip.y - nearFarClip.x;

		float minZ = nearFarClip.x;
		float maxZ = nearFarClip.x + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		DirectionalLightComponent shadowLight = m_Scene->GetDirectionalLightData(0);

		// Calculate Split Depths based on View Camera Frustum
		// Based on Method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; ++i)
		{
			float p = (i + 1) / (float)SHADOW_MAP_CASCADE_COUNT;
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = m_CSMSettings.CascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearFarClip.x) / clipRange;
		}

		// Calculate Orthographic Projection Matrix for each Cascade
		float lastSplitDist = 0.0f;
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; ++i)
		{
			float splitDist = cascadeSplits[i];

			glm::vec3 frustumCorners[8] = {
				glm::vec3(-1.0f,  1.0f, 0.0f),
				glm::vec3( 1.0f,  1.0f, 0.0f),
				glm::vec3( 1.0f, -1.0f, 0.0f),
				glm::vec3(-1.0f, -1.0f, 0.0f),
				glm::vec3(-1.0f,  1.0f, 1.0f),
				glm::vec3( 1.0f,  1.0f, 1.0f),
				glm::vec3( 1.0f, -1.0f, 1.0f),
				glm::vec3(-1.0f, -1.0f, 1.0f)
			};

			// Project Frustum Corners into World Space
			glm::mat4 invCam = glm::inverse(cameraData.GetProjectionMatrix() * cameraData.GetViewMatrix());
			for (uint32_t j = 0; j < 8; ++j)
			{
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[j], 1.0f);
				frustumCorners[j] = invCorner / invCorner.w;
			}

			for (uint32_t j = 0; j < 4; ++j)
			{
				glm::vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
				frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
				frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
			}

			// Get Frustum Center
			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t j = 0; j < 8; ++j)
				frustumCenter += frustumCorners[j];

			frustumCenter /= 8.0f;

			float radius = 0.0f;
			for (uint32_t j = 0; j < 8; ++j)
			{
				float distance = glm::length(frustumCorners[j] - frustumCenter);
				radius = glm::max(radius, distance);
			}

			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::vec3 lightDir = glm::normalize(shadowLight.Direction);
			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y,
				m_CSMSettings.CascadeNearPlaneOffset, maxExtents.z - minExtents.z + m_CSMSettings.CascadeFarPlaneOffset);

			if (m_CSMSettings.CascadeOffset)
			{
				float shadowMapSize = (float)m_CSMSettings.MapSize;
				glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
				glm::vec4 shadowOrigin = glm::vec4(m_CSMSettings.CascadeOrigin, 1.0f);
				shadowOrigin = shadowMatrix * shadowOrigin;
				shadowOrigin = shadowOrigin * shadowMapSize / 2.0f;

				glm::vec4 roundedOrigin = glm::round(shadowOrigin);
				glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
				roundOffset = roundOffset * 2.0f / shadowMapSize;
				roundOffset.z = 0.0f;
				roundOffset.w = 0.0f;

				glm::mat4 shadowProj = lightOrthoMatrix;
				shadowProj[3] += roundOffset;
				lightOrthoMatrix = shadowProj;
			}

			// Store Split Distance and matrix in Cascade
			m_CascadeData.CascadeSplitLevels[i] = (nearFarClip.x + splitDist * clipRange) * -1.0f;
			m_CascadeData.CascadeLightMatrices[i] = lightOrthoMatrix * lightViewMatrix;

			lastSplitDist = cascadeSplits[i];
		}
	}

#if 0
	glm::mat4 SceneRenderer::GetLightSpaceMatrix(const float nearClip, const float farClip)
	{
		const auto& cameraData = m_SceneEditorData.CameraData;
		//const auto projectionMatrix = glm::perspective(cameraData.GetFieldOfView(), cameraData.GetAspectRatio(), nearClip, farClip);
		const auto frustumCorners = GetFrustumCornersWorldSpace();

		glm::vec3 center = m_ShadowLightPosition;
		for (auto& frustumCorner : frustumCorners)
			center += glm::vec3{ frustumCorner };

		center /= frustumCorners.size();

		const auto lightView = glm::lookAt(center + glm::normalize(m_ShadowLightDirection), center, glm::vec3{ 0.0f, 1.0f, 0.0f });

		float minX = std::numeric_limits<float>::max();
		float maxX = std::numeric_limits<float>::lowest();
		float minY = std::numeric_limits<float>::max();
		float maxY = std::numeric_limits<float>::lowest();
		float minZ = std::numeric_limits<float>::max();
		float maxZ = std::numeric_limits<float>::lowest();
		for (const auto& frustumCorner : frustumCorners)
		{
			const auto transform = lightView * frustumCorner;
			minX = std::min(minX, transform.x);
			maxX = std::max(maxX, transform.x);
			minY = std::min(minY, transform.y);
			maxY = std::max(maxY, transform.y);
			minZ = std::min(minZ, transform.z);
			maxZ = std::max(maxZ, transform.z);
		}

		if (minZ < 0)
			minZ *= m_DepthFactor;
		else
			minZ /= m_DepthFactor;
		if (maxZ < 0)
			maxZ /= m_DepthFactor;
		else
			maxZ *= m_DepthFactor;

		const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
		return lightProjection * lightView;
	}

	std::vector<glm::mat4> SceneRenderer::GetLightSpaceMatrices()
	{
		const glm::vec2 cameraNearFarClip = m_SceneEditorData.CameraData.GetNearFarClip();

		std::vector<glm::mat4> lightSpaceMatrices{ GetLightSpaceMatrix(cameraNearFarClip.x, m_CascadeLevels[0]) };
		for (size_t i = 1; i < m_CascadeLevels.size(); ++i)
			lightSpaceMatrices.push_back(GetLightSpaceMatrix(m_CascadeLevels[i - 1], m_CascadeLevels[i]));

		lightSpaceMatrices.push_back(GetLightSpaceMatrix(m_CascadeLevels.back(), cameraNearFarClip.y));
		return lightSpaceMatrices;
	}
#endif

	void SceneRenderer::CreateResources()
	{
		auto device = VulkanContext::GetCurrentDevice();

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		Renderer::WaitAndExecute();

		// Create Physics Debug Renderer
		m_PhysicsDebugRenderer = PhysicsDebugRenderer::Create();

		m_SceneImages.resize(framesInFlight);
		m_ShadowDepthPassImages.resize(framesInFlight);

		m_UBCamera.reserve(framesInFlight);
		m_UBPointLight.reserve(framesInFlight);
		m_UBSpotLight.reserve(framesInFlight);
		m_UBDirectionalLight.reserve(framesInFlight);
		m_UBCascadeLightMatrices.reserve(framesInFlight);
		m_ImageBuffer.reserve(framesInFlight);

		m_BloomMipSize = (glm::uvec2(m_ViewportSize.x, m_ViewportSize.y) + 1u) / 2u;
		m_BloomMipSize += 16u - m_BloomMipSize % 16u;

		m_BloomTextures.reserve(framesInFlight);
		m_AOTextures.reserve(framesInFlight);
		m_SceneRenderTextures.reserve(framesInFlight);
#if VK_FEATURE_GTAO
		m_SceneDepthTextures.reserve(framesInFlight);
#endif

		VkCommandBuffer barrierCmd = device->GetCommandBuffer();

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			// ImGui Images
			std::shared_ptr<VulkanImage> finalPassImage = std::dynamic_pointer_cast<VulkanImage>(GetFinalPassImage(i));
			m_SceneImages[i] = ImGuiLayer::AddTexture(*finalPassImage);

			auto shadowMapImage = m_ShadowMapPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer->GetDepthAttachment()[i];
			m_ShadowDepthPassImages[i] = ImGuiLayer::AddTexture(*std::dynamic_pointer_cast<VulkanImage>(shadowMapImage), 0);

			// Uniform Buffers
			m_UBCamera.emplace_back(std::make_shared<VulkanUniformBuffer>(sizeof(UBCamera)));
			m_UBPointLight.emplace_back(std::make_shared<VulkanUniformBuffer>(sizeof(UBPointLights)));
			m_UBSpotLight.emplace_back(std::make_shared<VulkanUniformBuffer>(sizeof(UBSpotLights)));
			m_UBDirectionalLight.emplace_back(std::make_shared<VulkanUniformBuffer>(sizeof(UBDirectionalLights)));
			m_UBCascadeLightMatrices.emplace_back(std::make_shared<VulkanUniformBuffer>(sizeof(CascadeMapData)));
			m_ImageBuffer.emplace_back(std::make_shared<VulkanIndexBuffer>(m_ViewportSize.x * m_ViewportSize.y * sizeof(int)));

			// Bloom Compute Textures
			ImageSpecification bloomRTSpec{};
			bloomRTSpec.DebugName = std::format("Bloom Compute Texture {}", i);
			bloomRTSpec.Width = m_BloomMipSize.x;
			bloomRTSpec.Height = m_BloomMipSize.y;
			bloomRTSpec.Format = ImageFormat::RGBA32F;
			bloomRTSpec.Usage = ImageUsage::Storage;
			bloomRTSpec.MipLevels = Utils::CalculateMipCount(m_BloomMipSize.x, m_BloomMipSize.y) - 2;

			auto BloomTexture = std::static_pointer_cast<VulkanImage>(m_BloomTextures.emplace_back(std::make_shared<VulkanImage>(bloomRTSpec)));
			BloomTexture->Invalidate();

			// AO Textures
			ImageSpecification aoTexSpec{};
			aoTexSpec.DebugName = std::format("AO Texture {}", i);
			aoTexSpec.Width = m_ViewportSize.x;
			aoTexSpec.Height = m_ViewportSize.y;
			aoTexSpec.Format = ImageFormat::RGBA32F;
			aoTexSpec.Usage = ImageUsage::Storage;

			auto AOTexture = std::static_pointer_cast<VulkanImage>(m_AOTextures.emplace_back(std::make_shared<VulkanImage>(aoTexSpec)));
			AOTexture->Invalidate();

			// Scene Render Textures
			ImageSpecification sceneRTSpec{};
			sceneRTSpec.DebugName = std::format("Scene Render Texture {}", i);
			sceneRTSpec.Width = m_ViewportSize.x;
			sceneRTSpec.Height = m_ViewportSize.y;
			sceneRTSpec.Format = ImageFormat::RGBA32F;
			sceneRTSpec.Usage = ImageUsage::Texture;
			sceneRTSpec.MipLevels = Utils::CalculateMipCount(m_ViewportSize.x, m_ViewportSize.y);

			auto SceneTexture = std::static_pointer_cast<VulkanImage>(m_SceneRenderTextures.emplace_back(std::make_shared<VulkanImage>(sceneRTSpec)));
			SceneTexture->Invalidate();

			Utils::InsertImageMemoryBarrier(barrierCmd, SceneTexture->GetVulkanImageInfo().Image,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, SceneTexture->GetSpecification().MipLevels, 0, 1 });

#if VK_FEATURE_GTAO
			// Scene Depth Textures
			ImageSpecification sceneDepthSpec = {};
			sceneDepthSpec.DebugName = std::format("Scene Depth Texture {}", i);
			sceneDepthSpec.Width = m_ViewportSize.x;
			sceneDepthSpec.Height = m_ViewportSize.y;
			sceneDepthSpec.Format = ImageFormat::DEPTH32F;
			sceneDepthSpec.Usage = ImageUsage::Texture;
			sceneDepthSpec.MipLevels = sceneRTSpec.MipLevels;
			sceneDepthSpec.SamplerFilter = FilterMode::Nearest;

			auto SceneDepthTexture = std::static_pointer_cast<VulkanImage>(m_SceneDepthTextures.emplace_back(std::make_shared<VulkanImage>(sceneDepthSpec)));
			SceneDepthTexture->Invalidate();

			Utils::InsertImageMemoryBarrier(barrierCmd, SceneDepthTexture->GetVulkanImageInfo().Image,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 });
#endif
		}

		device->FlushCommandBuffer(barrierCmd);

		m_BloomDirtTexture = AssetManager::GetAsset<Texture2D>("textures/LensDirt.png");

		auto blackTextureCube = std::dynamic_pointer_cast<VulkanTextureCube>(Renderer::GetBlackTextureCube(ImageFormat::RGBA8_UNORM));
		blackTextureCube->Invalidate();

		m_CubemapTexture = blackTextureCube;
		m_PrefilteredTexture = blackTextureCube;
		m_IrradianceTexture = blackTextureCube;

		m_SkyboxTextureID = ImGuiLayer::AddTexture(*std::dynamic_pointer_cast<VulkanTextureCube>(m_PrefilteredTexture));

		m_BRDFTexture = Renderer::CreateBRDFTexture();
		m_PointLightTextureIcon = TextureImporter::LoadTexture2D("../../EditorLayer/Resources/Icons/PointLightIcon.png");
		m_SpotLightTextureIcon = TextureImporter::LoadTexture2D("../../EditorLayer/Resources/Icons/SpotLightIcon.png");
	}

	void SceneRenderer::Release()
	{
	}

	void SceneRenderer::RecreateScene()
	{
		auto device = VulkanContext::GetCurrentDevice();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		VK_CORE_INFO("Scene Resized!");
		m_GeometryPipeline->GetSpecification().pRenderPass->RecreateFramebuffers(m_ViewportSize.x, m_ViewportSize.y);
		m_GeometrySelectPipeline->GetSpecification().pRenderPass->RecreateFramebuffers(m_ViewportSize.x, m_ViewportSize.y);
		m_CompositePipeline->GetSpecification().pRenderPass->RecreateFramebuffers(m_ViewportSize.x, m_ViewportSize.y);

		// Recreate Resources
		{
			m_BloomMipSize = (glm::uvec2(m_ViewportSize.x, m_ViewportSize.y) + 1u) / 2u;
			m_BloomMipSize += 16u - m_BloomMipSize % 16u;

			VkCommandBuffer barrierCmd = device->GetCommandBuffer();

			for (uint32_t i = 0; i < framesInFlight; ++i)
			{
				// General Textures
				m_BloomTextures[i]->Resize(m_BloomMipSize.x, m_BloomMipSize.y, Utils::CalculateMipCount(m_BloomMipSize.x, m_BloomMipSize.y) - 2);
				m_AOTextures[i]->Resize(m_ViewportSize.x, m_ViewportSize.y);

				auto vulkanSceneTexture = std::dynamic_pointer_cast<VulkanImage>(m_SceneRenderTextures[i]);
				vulkanSceneTexture->Resize(m_ViewportSize.x, m_ViewportSize.y, Utils::CalculateMipCount(m_ViewportSize.x, m_ViewportSize.y));
#if VK_FEATURE_GTAO
				auto vulkanDepthTexture = std::dynamic_pointer_cast<VulkanImage>(m_SceneDepthTextures[i]);
				vulkanDepthTexture->Resize(m_ViewportSize.x, m_ViewportSize.y, Utils::CalculateMipCount(m_ViewportSize.x, m_ViewportSize.y));
#endif

				Utils::InsertImageMemoryBarrier(barrierCmd, vulkanSceneTexture->GetVulkanImageInfo().Image,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, vulkanSceneTexture->GetSpecification().MipLevels, 0, 1 });

#if VK_FEATURE_GTAO
				Utils::InsertImageMemoryBarrier(barrierCmd, vulkanDepthTexture->GetVulkanImageInfo().Image,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, vulkanDepthTexture->GetSpecification().MipLevels, 0, 1 });
#endif

				// Update ImGui Viewport Image
				std::shared_ptr<VulkanImage> finalPassImage = std::dynamic_pointer_cast<VulkanImage>(GetFinalPassImage(i));
				ImGuiLayer::UpdateDescriptor(m_SceneImages[i], *finalPassImage);
			}

			device->FlushCommandBuffer(barrierCmd);
		}

		// Recreate Shader Materials
		RecreateMaterials();
	}

	void SceneRenderer::OnImGuiRender()
	{
		ImGui::Begin("Scene Renderer");

		const ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (ImGui::TreeNodeEx("Scene Settings", treeFlags))
		{
			ImGui::DragFloat("Exposure Intensity", &m_SceneSettings.Exposure, 0.01f, 0.0f, 20.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragFloat("Dirt Intensity", &m_SceneSettings.DirtIntensity, 0.01f, 0.0f, 100.0f);
			ImGui::DragFloat("Skybox LOD", &m_SkyboxSettings.LOD, 0.01f, 0.0f, 11.0f);
			ImGui::DragFloat("Skybox Intensity", &m_SkyboxSettings.Intensity, 0.01f, 0.0f, 20.0f);
			ImGui::Checkbox("Fog", (bool*)&m_SceneSettings.Fog);

			ImGui::TreePop();
		}

		if (m_SceneSettings.Fog)
		{
			ImGui::BeginChild("##FogSettings", { 0, 0 }, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);
			
			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 100.0f);
			ImGui::DragFloat("Start Distance", &m_SceneSettings.FogStartDistance, 0.01f, 0.01f);
			ImGui::DragFloat("Falloff Distance", &m_SceneSettings.FogFallOffDistance, 0.01f, 0.1f);
			ImGui::PopItemWidth();

			ImGui::EndChild();
		}

		if (ImGui::TreeNodeEx("Scene Renderer Stats##GPUPerf", treeFlags))
		{
			auto vulkanCmdBuffer = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer);

			ImGui::Text("Geometry Pass: %lluns", vulkanCmdBuffer->GetQueryTime(1));
			ImGui::Text("Skybox Pass: %lluns", vulkanCmdBuffer->GetQueryTime(0));
			ImGui::Text("Lights Pass: %lluns", vulkanCmdBuffer->GetQueryTime(2));
			ImGui::Text("Composite Pass: %lluns", vulkanCmdBuffer->GetQueryTime(4));
			ImGui::Text("Bloom Compute Pass: %lluns", vulkanCmdBuffer->GetQueryTime(3));
			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Bloom Settings", treeFlags))
		{
			ImGui::DragFloat("Threshold", &m_BloomParams.Threshold, 0.01f, 0.0f, 1000.0f);
			ImGui::DragFloat("Knee", &m_BloomParams.Knee, 0.01f, 0.001f, 1.0f);

			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Shadow Settings", treeFlags))
		{
			ImGui::DragFloat("Cascade Split Lambda", &m_CSMSettings.CascadeSplitLambda, 0.001f, 0.0f, 1.0f);
			ImGui::DragFloat("Cascade Near Offset", &m_CSMSettings.CascadeNearPlaneOffset, 0.01f, 0.05f, 1000.0f);
			ImGui::DragFloat("Cascade Far Offset", &m_CSMSettings.CascadeFarPlaneOffset, 0.01f, 0.05f, 1000.0f);
			ImGui::DragFloat3("Cascade Origin", glm::value_ptr(m_CSMSettings.CascadeOrigin), 0.01f);
			ImGui::Checkbox("Cascade Offset", (bool*)&m_CSMSettings.CascadeOffset);

			ImVec2 quadSize = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().x };
			ImGui::Image(m_ShadowDepthPassImages[Renderer::RT_GetCurrentFrameIndex()], quadSize);

			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Scene Shader Map", treeFlags))
		{
			auto& shaderMap = Renderer::GetShaderMap();

			int buttonID = 0;
			for (auto&& [name, shader] : shaderMap)
			{
				ImGui::SetNextItemAllowOverlap();
				ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_AllowOverlap, ImVec2{ 0.0f, 24.5f });

				ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
				ImGui::PushID(buttonID++);
				if (ImGui::Button("Reload", ImVec2{ 0.0f, 24.5f }))
				{
					shader->Reload();
					RecreatePipelines();
				}

				ImGui::PopID();
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Scene Draw/Allocation Stats", treeFlags))
		{
			auto renderStats = VulkanRenderer::GetRendererStats();

			ImGui::Text("Draw Calls: %u", renderStats.DrawCalls);
			ImGui::Text("Instance Count: %u", renderStats.InstanceCount);
			ImGui::Text("Draw Calls Saved: %u", renderStats.InstanceCount - renderStats.DrawCalls);
			ImGui::Text("Total Allocated Bytes: %u", VulkanAllocator::GetAllocationStats().AllocatedBytes);
			ImGui::Text("Total Allocation Count: %u", VulkanAllocator::GetAllocationStats().AllocationCount);
			ImGui::TreePop();
		}

		ImGui::End(); // End of Scene Renderer Window

		VulkanRenderer::ResetStats();
	}

	void SceneRenderer::SetActiveScene(std::shared_ptr<Scene> scene)
	{
		m_Scene = scene;

		m_MeshDrawList.clear();
		m_MeshTessellatedDrawList.clear();
		m_MeshTransformMap.clear();
	}

	void SceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
	{
		m_ViewportSize.x = width;
		m_ViewportSize.y = height;

		RecreateScene();
	}

	void SceneRenderer::RenderScene()
	{
		VK_CORE_PROFILE_FN("Submit-SceneRenderer");

		int frameIndex = Renderer::GetCurrentFrameIndex();

		// Camera
		UBCamera cameraUB{};
		cameraUB.Projection = m_SceneEditorData.CameraData.GetProjectionMatrix();
		cameraUB.View = m_SceneEditorData.CameraData.GetViewMatrix();
		cameraUB.InverseView = m_SceneEditorData.CameraData.GetInverseViewMatrix();

		glm::vec2 nearFarClip = m_SceneEditorData.CameraData.GetNearFarClip();
		cameraUB.DepthUnpackConsts.x = (nearFarClip.y * nearFarClip.x) / (nearFarClip.y - nearFarClip.x);
		cameraUB.DepthUnpackConsts.y = (nearFarClip.y + nearFarClip.x) / (nearFarClip.y - nearFarClip.x);

		m_UBCamera[frameIndex]->WriteData(&cameraUB);

		// Lights
		UBPointLights pointLightUB{};
		UBSpotLights spotLightUB{};
		UBDirectionalLights directionLightUB{};
		m_Scene->UpdateLightsBuffer(pointLightUB, spotLightUB, directionLightUB);

		m_UBPointLight[frameIndex]->WriteData(&pointLightUB);
		m_UBSpotLight[frameIndex]->WriteData(&spotLightUB);
		m_UBDirectionalLight[frameIndex]->WriteData(&directionLightUB);

		UpdateCascadeMap();
		m_UBCascadeLightMatrices[frameIndex]->WriteData(&m_CascadeData);

		m_SceneCommandBuffer->Begin();

		ShadowPass();
		GeometryPass();
		GTAOCompute();
		BloomCompute();
		CompositePass();

		if (m_SceneEditorData.ViewportHovered)
			SelectionPass();

		ResetDrawCommands();

		m_SceneCommandBuffer->End();
	}

	void SceneRenderer::RenderLights()
	{ 
		Renderer::BindPipeline(m_SceneCommandBuffer, m_LightPipeline, m_PointLightShaderMaterial);

		// Point Lights
		for (auto pointLightPosition : m_PointLightPositions)
			Renderer::RenderLight(m_SceneCommandBuffer, m_LightPipeline, pointLightPosition);

		Renderer::BindPipeline(m_SceneCommandBuffer, m_LightPipeline, m_SpotLightShaderMaterial);

		// Spot Lights
		for (auto spotLightPosition : m_SpotLightPositions)
			Renderer::RenderLight(m_SceneCommandBuffer, m_LightPipeline, spotLightPosition);
	}

	void SceneRenderer::SubmitMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialTable>& materialTable, const glm::mat4& transform)
	{
		VK_CORE_PROFILE();

		if (!mesh || !materialTable)
			return;

		auto meshSource = mesh->GetMeshSource();
		auto& submeshData = meshSource->GetSubmeshes();
		uint64_t meshHandle = mesh->Handle;

		if (meshSource->GetVertexCount() < 1)
			return;

		for (uint32_t submeshIndex : mesh->GetSubmeshes())
		{
			int materialIndex = submeshData[submeshIndex].MaterialIndex;
			std::shared_ptr<MaterialAsset> materialAsset = materialTable->HasMaterial(materialIndex) ? materialTable->GetMaterial(materialIndex) : meshSource->GetBaseMaterial();

			uint64_t materialHandle = materialAsset->Handle;
			bool isTessellated = materialAsset->HasDisplacementTexture();

			MeshKey meshKey = { meshHandle, materialHandle, submeshIndex };
			auto& transformBuffer = isTessellated ? m_MeshTessellatedTransformMap[meshKey].Transforms.emplace_back()
				: m_MeshTransformMap[meshKey].Transforms.emplace_back();

			glm::mat4 submeshTransform = transform * submeshData[submeshIndex].LocalTransform;
			transformBuffer.MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			transformBuffer.MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			transformBuffer.MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };

			auto& dc = isTessellated ? m_MeshTessellatedDrawList[meshKey] : m_MeshDrawList[meshKey];
			dc.MeshInstance = mesh;
			dc.MaterialInstance = materialAsset->GetMaterial();
			dc.SubmeshIndex = submeshIndex;
			dc.TransformBuffer = isTessellated ? m_MeshTessellatedTransformMap[meshKey].TransformBuffer : m_MeshTransformMap[meshKey].TransformBuffer;
			dc.InstanceCount++;

			if (isTessellated && m_MeshDrawList.contains(meshKey))
				m_MeshDrawList.erase(meshKey);
			else if (!isTessellated && m_MeshTessellatedDrawList.contains(meshKey))
				m_MeshTessellatedDrawList.erase(meshKey);
		}
	}

	void SceneRenderer::SubmitSelectedMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialTable>& materialTable, const glm::mat4& transform, uint32_t entityID)
	{
		VK_CORE_PROFILE();

		if (!mesh || !materialTable)
			return;

		auto meshSource = mesh->GetMeshSource();
		auto& submeshData = meshSource->GetSubmeshes();
		uint64_t meshHandle = mesh->Handle;

		if (meshSource->GetVertexCount() < 1)
			return;

		for (uint32_t submeshIndex : mesh->GetSubmeshes())
		{
			int materialIndex = submeshData[submeshIndex].MaterialIndex;
			std::shared_ptr<MaterialAsset> materialAsset = materialTable->HasMaterial(materialIndex) ? materialTable->GetMaterial(materialIndex) : meshSource->GetBaseMaterial();

			uint64_t materialHandle = materialAsset->Handle;
			MeshKey meshKey = { meshHandle, materialHandle, submeshIndex };
			auto& transformBuffer = m_SelectedMeshTransformMap[meshKey].Transforms.emplace_back();

			glm::mat4 submeshTransform = transform * submeshData[submeshIndex].LocalTransform;
			transformBuffer.MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			transformBuffer.MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			transformBuffer.MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };
			transformBuffer.EntityID = entityID;

			auto& dc = m_SelectedMeshDrawList[meshKey];
			dc.MeshInstance = mesh;
			dc.SubmeshIndex = submeshIndex;
			dc.TransformBuffer = m_SelectedMeshTransformMap[meshKey].TransformBuffer;
			dc.InstanceCount++;
		}
	}

	void SceneRenderer::SubmitTransparentMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialTable>& materialTable, const glm::mat4& transform)
	{

	}

	void SceneRenderer::SubmitPhysicsMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const glm::mat4& transform)
	{
		VK_CORE_PROFILE();

		if (!mesh || !materialAsset)
			return;

		auto meshSource = mesh->GetMeshSource();
		auto& submeshData = meshSource->GetSubmeshes();
		uint64_t meshHandle = mesh->Handle;

		if (meshSource->GetVertexCount() < 1)
			return;

		for (uint32_t submeshIndex : mesh->GetSubmeshes())
		{
			int materialIndex = submeshData[submeshIndex].MaterialIndex;
			uint64_t materialHandle = materialAsset->Handle;

			MeshKey meshKey = { meshHandle, materialHandle, submeshIndex };
			auto& transformBuffer = m_PhysicsDebugMeshTransformMap[meshKey].Transforms.emplace_back();

			glm::mat4 submeshTransform = transform * submeshData[submeshIndex].LocalTransform;
			transformBuffer.MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			transformBuffer.MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			transformBuffer.MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };

			auto& dc = m_PhysicsDebugMeshDrawList[meshKey];
			dc.MeshInstance = mesh;
			dc.MaterialInstance = materialAsset->GetMaterial();
			dc.SubmeshIndex = submeshIndex;
			dc.TransformBuffer = m_PhysicsDebugMeshTransformMap[meshKey].TransformBuffer;
			dc.InstanceCount++;
		}
	}

	void SceneRenderer::UpdateMeshInstanceData(std::shared_ptr<Mesh> mesh, std::shared_ptr<MaterialTable> materialTable)
	{
		auto meshSource = mesh->GetMeshSource();
		auto& submeshData = meshSource->GetSubmeshes();
		uint64_t meshHandle = mesh->Handle;

		for (uint32_t submeshIndex : mesh->GetSubmeshes())
		{
			int materialIndex = submeshData[submeshIndex].MaterialIndex;
			std::shared_ptr<MaterialAsset> materialAsset = materialTable->HasMaterial(materialIndex) ? materialTable->GetMaterial(materialIndex) : meshSource->GetBaseMaterial();

			uint64_t materialHandle = materialAsset->Handle;
			MeshKey meshKey = { meshHandle, materialHandle, submeshIndex };
			m_MeshTransformMap.erase(meshKey);
			m_MeshDrawList.erase(meshKey);
		}
	}

	std::shared_ptr<Image2D> SceneRenderer::GetFinalPassImage(uint32_t index) const
	{
		auto framebuffer = std::dynamic_pointer_cast<VulkanFramebuffer>(m_SceneFramebuffer);
		return framebuffer->GetAttachment()[index];
	}
		
	void SceneRenderer::UpdateSkybox(AssetHandle skyTextureHandle)
	{
		auto equirectTexture = AssetManager::GetAsset<Texture2D>(skyTextureHandle);

		// Obtain Cubemaps
		auto [filteredMap, irradianceMap] = VulkanRenderer::CreateEnviromentMap(equirectTexture);
		m_CubemapTexture = filteredMap;
		m_PrefilteredTexture = filteredMap;
		m_IrradianceTexture = irradianceMap;

		// Update Materials
		m_GeometryMaterial->SetTexture(6, m_IrradianceTexture);
		m_GeometryMaterial->SetTexture(8, m_PrefilteredTexture);
		m_GeometryMaterial->PrepareShaderMaterial();

		m_SkyboxMaterial->SetTexture(1, m_CubemapTexture);
		m_SkyboxMaterial->PrepareShaderMaterial();

		ImGuiLayer::UpdateDescriptor(m_SkyboxTextureID, *std::dynamic_pointer_cast<VulkanTextureCube>(m_PrefilteredTexture));
	}

	void SceneRenderer::SetSkybox(AssetHandle skyTextureHandle)
	{
		s_Instance->UpdateSkybox(skyTextureHandle);
	}

	void SceneRenderer::CompositePass()
	{
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Composite-Pass", DebugLabelColor::Red);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		Renderer::BeginRenderPass(m_SceneCommandBuffer, m_CompositePipeline->GetSpecification().pRenderPass);

		Renderer::Submit([this]
		{
			auto vulkanCmdBuffer = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer);
			auto vulkanCompPipeline = std::static_pointer_cast<VulkanPipeline>(m_CompositePipeline);

			vulkanCompPipeline->SetPushConstants(vulkanCmdBuffer->RT_GetActiveCommandBuffer(), &m_SceneSettings, sizeof(SceneSettings));
		});

		Renderer::SubmitFullscreenQuad(m_SceneCommandBuffer, m_CompositePipeline, m_CompositeShaderMaterial);
		Renderer::EndRenderPass(m_SceneCommandBuffer, m_CompositePipeline->GetSpecification().pRenderPass);

		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
	}

	void SceneRenderer::ShadowPass()
	{
		m_Scene->OnUpdateGeometry(this);
		m_Scene->OnUpdateLights(m_PointLightPositions, m_SpotLightPositions, m_LightHandles);

		// This method will call inherited Jolt Renderer methods
		// Then Submit and Flush to Vertex Buffers
		if (m_SceneEditorData.ShowPhysicsCollider)
		{
			m_Scene->DrawPhysicsBodies(m_PhysicsDebugRenderer);
			m_PhysicsDebugRenderer->FlushData();
		}

		Renderer::BeginRenderPass(m_SceneCommandBuffer, m_ShadowMapPipeline->GetSpecification().pRenderPass);
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "ShadowPass", DebugLabelColor::Aqua);
		Renderer::BindPipeline(m_SceneCommandBuffer, m_ShadowMapPipeline, m_ShadowMapMaterial);

		for (auto& [mk, dc] : m_MeshDrawList)
			Renderer::RenderMeshWithoutMaterial(m_SceneCommandBuffer, dc.MeshInstance, dc.SubmeshIndex, dc.TransformBuffer, m_MeshTransformMap[mk].Transforms, dc.InstanceCount);

		Renderer::EndRenderPass(m_SceneCommandBuffer, m_ShadowMapPipeline->GetSpecification().pRenderPass);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
	}

	void SceneRenderer::GeometryPass()
	{
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Skybox", DebugLabelColor::Orange);

		Renderer::BeginRenderPass(m_SceneCommandBuffer, m_GeometryPipeline->GetSpecification().pRenderPass);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		if (m_SceneEditorData.ShowPhysicsCollider)
		{
			Renderer::BindPipeline(m_SceneCommandBuffer, m_LinesPipeline, m_LinesMaterial);
			m_PhysicsDebugRenderer->Draw(m_SceneCommandBuffer);
		}

		// Rendering Skybox
		Renderer::RenderSkybox(m_SceneCommandBuffer, m_SkyboxPipeline, m_SkyboxMaterial, &m_SkyboxSettings);
		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);

		// Rendering Geometry
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Geometry-Pass", DebugLabelColor::Gold);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		Renderer::BindPipeline(m_SceneCommandBuffer, m_GeometryPipeline, m_GeometryMaterial);

		// Static Geometry
		for (auto& [mk, dc] : m_MeshDrawList)
			Renderer::RenderMesh(m_SceneCommandBuffer, dc.MeshInstance, dc.MaterialInstance, dc.SubmeshIndex, m_GeometryPipeline, dc.TransformBuffer, m_MeshTransformMap[mk].Transforms, dc.InstanceCount);

		if (m_SceneEditorData.ShowPhysicsCollider)
		{
			// Physics Debug Geometry
			for (auto& [mk, dc] : m_PhysicsDebugMeshDrawList)
				Renderer::RenderMesh(m_SceneCommandBuffer, dc.MeshInstance, dc.MaterialInstance, dc.SubmeshIndex, m_GeometryPipeline, dc.TransformBuffer, m_PhysicsDebugMeshTransformMap[mk].Transforms, dc.InstanceCount);
		}

		// Tessellated Geometry(i.e. Displacement Maps, Dynamic LOD)
		Renderer::BindPipeline(m_SceneCommandBuffer, m_GeometryTessellatedPipeline, m_GeometryMaterial);

		for (auto& [mk, dc] : m_MeshTessellatedDrawList)
			Renderer::RenderMesh(m_SceneCommandBuffer, dc.MeshInstance, dc.MaterialInstance, dc.SubmeshIndex, m_GeometryTessellatedPipeline, dc.TransformBuffer, m_MeshTessellatedTransformMap[mk].Transforms, dc.InstanceCount);

		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);

		// TODO: Render Transparent Meshes

		// Rendering Point Lights
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Lights", DebugLabelColor::Grey);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		RenderLights();

		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);

		Renderer::EndRenderPass(m_SceneCommandBuffer, m_GeometryPipeline->GetSpecification().pRenderPass);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);

		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Copy-Image", DebugLabelColor::Pink);

		// Copying Image for Bloom
		int frameIndex = Renderer::GetCurrentFrameIndex();

		Renderer::CopyVulkanImage(m_SceneCommandBuffer,
			m_GeometryPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer->GetAttachment()[frameIndex],
			m_SceneRenderTextures[frameIndex]);

		Renderer::BlitVulkanImage(m_SceneCommandBuffer, m_SceneRenderTextures[frameIndex]);
#if VK_FEATURE_GTAO
		Renderer::CopyVulkanImage(m_SceneCommandBuffer,
			m_GeometryPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer->GetDepthAttachment()[frameIndex],
			m_SceneDepthTextures[frameIndex]);

		Renderer::BlitVulkanImage(m_SceneCommandBuffer, m_SceneDepthTextures[frameIndex]);
#endif

		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
	}

	void SceneRenderer::SelectionPass()
	{
		m_Scene->OnSelectGeometry(this);

		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Selection-Pass", DebugLabelColor::Green);
		Renderer::BeginRenderPass(m_SceneCommandBuffer, m_GeometrySelectPipeline->GetSpecification().pRenderPass);
		Renderer::BindPipeline(m_SceneCommandBuffer, m_GeometrySelectPipeline, m_GeometrySelectMaterial);

		for (auto& [mk, dc] : m_SelectedMeshDrawList)
			Renderer::RenderSelectedMesh(m_SceneCommandBuffer, dc.MeshInstance, dc.SubmeshIndex, dc.TransformBuffer, m_SelectedMeshTransformMap[mk].Transforms, dc.InstanceCount);

		// Lights
		{
			Renderer::BindPipeline(m_SceneCommandBuffer, m_LightSelectPipeline, m_LightSelectMaterial);

			int index = 0;
			// Point Lights
			for (auto& pointLightPosition : m_PointLightPositions)
				Renderer::RenderLight(m_SceneCommandBuffer, m_LightSelectPipeline, { pointLightPosition, (int)m_LightHandles[index++] });

			// Spot Lights
			for (auto& spotLightPosition : m_SpotLightPositions)
				Renderer::RenderLight(m_SceneCommandBuffer, m_LightSelectPipeline, { spotLightPosition, (int)m_LightHandles[index++] });
		}

		Renderer::EndRenderPass(m_SceneCommandBuffer, m_GeometrySelectPipeline->GetSpecification().pRenderPass);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);

		Renderer::Submit([this]
		{
			int frameIndex = Renderer::RT_GetCurrentFrameIndex();
			auto frameBuffer = std::static_pointer_cast<VulkanFramebuffer>(m_GeometrySelectPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer);

			void* pixelData = frameBuffer->ReadPixel(m_SceneCommandBuffer, m_ImageBuffer[frameIndex], 0, m_SceneEditorData.ViewportMousePos.x, m_SceneEditorData.ViewportMousePos.y);
			m_HoveredEntity = *(int*)pixelData;
		});
	}

	void SceneRenderer::GTAOCompute()
	{
		Renderer::Submit([this]
		{
			auto vulkanGTAOPipeline = std::static_pointer_cast<VulkanComputePipeline>(m_GTAOPipeline);
			auto vulkanCmdBuffer = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer);

			VkCommandBuffer dispatchCmd = vulkanCmdBuffer->RT_GetActiveCommandBuffer();
			vulkanGTAOPipeline->Bind(dispatchCmd);

			glm::uvec2 workGroups = glm::ceil((glm::vec2)m_ViewportSize / 32.0f);
			
			m_GTAOMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_GTAOPipeline);
			vulkanGTAOPipeline->Dispatch(dispatchCmd, workGroups.x, workGroups.y, 1);
		});
	}

	void SceneRenderer::BloomCompute()
	{
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Bloom", DebugLabelColor::Blue);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		Renderer::Submit([this]
		{
			VK_CORE_PROFILE_FN("SceneRenderer::BloomCompute");

			auto vulkanBloomPipeline = std::static_pointer_cast<VulkanComputePipeline>(m_BloomPipeline);
			auto vulkanCmdBuffer = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer);

			VkCommandBuffer dispatchCmd = vulkanCmdBuffer->RT_GetActiveCommandBuffer();
			vulkanBloomPipeline->Bind(dispatchCmd);

			// Prefilter
			m_LodAndMode.LOD = 0.0f;
			m_LodAndMode.Mode = 0.0f;

			m_BloomComputeMaterials.PrefilterMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);

			const uint32_t mips = m_BloomTextures[0]->GetSpecification().MipLevels;
			glm::uvec2 workGroups = glm::ceil((glm::vec2)m_BloomMipSize / 32.0f);

			vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
			vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_BloomParams, sizeof(glm::vec2), sizeof(glm::vec2));
			vulkanBloomPipeline->Dispatch(dispatchCmd, workGroups.x, workGroups.y, 1);

			for (uint32_t i = 1; i < mips; i++)
			{
				m_LodAndMode.LOD = float(i - 1);
				m_LodAndMode.Mode = 1.0f;

				int currentIdx = i - 1;

				m_BloomComputeMaterials.PingMaterials[currentIdx]->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);
				workGroups = glm::ceil((glm::vec2)m_BloomTextures[0]->GetMipSize(i) / 32.0f);

				vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
				vulkanBloomPipeline->Dispatch(dispatchCmd, workGroups.x, workGroups.y, 1);

				m_LodAndMode.LOD = (float)i;
				
				m_BloomComputeMaterials.PongMaterials[currentIdx]->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);

				vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
				vulkanBloomPipeline->Dispatch(dispatchCmd, workGroups.x, workGroups.y, 1);
			}

			// First Upsample
			m_LodAndMode.LOD = float(mips - 2);
			m_LodAndMode.Mode = 2.0f;

			m_BloomComputeMaterials.FirstUpsampleMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);
			workGroups = glm::ceil((glm::vec2)m_BloomTextures[2]->GetMipSize(mips - 1) / 32.0f);

			vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
			vulkanBloomPipeline->Dispatch(dispatchCmd, workGroups.x, workGroups.y, 1);

			// Upsample Final
			for (int i = mips - 2; i >= 0; --i)
			{
				m_LodAndMode.LOD = (float)i;
				m_LodAndMode.Mode = 3.0f;

				m_BloomComputeMaterials.UpsampleMaterials[i]->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);
				workGroups = glm::ceil((glm::vec2)m_BloomTextures[2]->GetMipSize(i) / 32.0f);

				vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
				vulkanBloomPipeline->Dispatch(dispatchCmd, workGroups.x, workGroups.y, 1);
			}
		});

		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
	}

	void SceneRenderer::CreateCommandBuffers()
	{
		auto device = VulkanContext::GetCurrentDevice();
		m_SceneCommandBuffer = std::make_shared<VulkanRenderCommandBuffer>(device->GetCommandPool(), CommandBufferLevel::Primary, 5);
	}

	void SceneRenderer::ResetDrawCommands()
	{
		for (auto& [mk, dc] : m_MeshDrawList)
		{
			m_MeshTransformMap[mk].Transforms.clear();
			dc.InstanceCount = 0;
		}

		for (auto& [mk, dc] : m_MeshTessellatedDrawList)
		{
			m_MeshTessellatedTransformMap[mk].Transforms.clear();
			dc.InstanceCount = 0;
		}

		for (auto& [mk, dc] : m_SelectedMeshDrawList)
		{
			m_SelectedMeshTransformMap[mk].Transforms.clear();
			dc.InstanceCount = 0;
		}

		for (auto& [mk, dc] : m_PhysicsDebugMeshDrawList)
		{
			m_PhysicsDebugMeshTransformMap[mk].Transforms.clear();
			dc.InstanceCount = 0;
		}

		if (m_SceneEditorData.ShowPhysicsCollider)
			m_PhysicsDebugRenderer->ClearData();

		m_PointLightPositions.clear();
		m_SpotLightPositions.clear();
		m_LightHandles.clear();
	}

}