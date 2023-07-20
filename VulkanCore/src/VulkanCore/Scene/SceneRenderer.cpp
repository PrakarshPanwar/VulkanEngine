#include "vulkanpch.h"
#include "SceneRenderer.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Core/Timer.h"
#include "VulkanCore/Core/ImGuiLayer.h"
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

#include <imgui.h>

namespace VulkanCore {

	namespace Utils {

		std::shared_ptr<VulkanVertexBuffer> CreateCubeModel()
		{
			float skyboxVertices[] = {       
				-1.0f,  1.0f, -1.0f,
				-1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				-1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f
			};

			return std::make_shared<VulkanVertexBuffer>(skyboxVertices, (uint32_t)sizeof(skyboxVertices));
		}

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
			{ ShaderDataType::Float3, "a_FragColor" },
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
			geomFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::DEPTH32F };
			geomFramebufferSpec.ReadDepthTexture = true;
			geomFramebufferSpec.Transfer = true;
			geomFramebufferSpec.Samples = 8;

			RenderPassSpecification geomRenderPassSpec;
			geomRenderPassSpec.TargetFramebuffer = std::make_shared<VulkanFramebuffer>(geomFramebufferSpec);

			PipelineSpecification geomPipelineSpec;
			geomPipelineSpec.DebugName = "Geometry Pipeline";
			geomPipelineSpec.pShader = Renderer::GetShader("CorePBR");
			geomPipelineSpec.Blend = true;
			geomPipelineSpec.pRenderPass = std::make_shared<VulkanRenderPass>(geomRenderPassSpec);
			geomPipelineSpec.Layout = vertexLayout;
			geomPipelineSpec.InstanceLayout = instanceLayout;

			PipelineSpecification lightPipelineSpec;
			lightPipelineSpec.DebugName = "Light Pipeline";
			lightPipelineSpec.pShader = Renderer::GetShader("LightShader");
			lightPipelineSpec.Blend = true;
			lightPipelineSpec.pRenderPass = geomPipelineSpec.pRenderPass;

			m_GeometryPipeline = std::make_shared<VulkanPipeline>(geomPipelineSpec);
			m_LightPipeline = std::make_shared<VulkanPipeline>(lightPipelineSpec);
		}

		// External Composite Pipeline
		{
			FramebufferSpecification extCompFramebufferSpec;
			extCompFramebufferSpec.Width = 1920;
			extCompFramebufferSpec.Height = 1080;
			extCompFramebufferSpec.Attachments = { ImageFormat::RGBA32F };
			extCompFramebufferSpec.Samples = 1;

			RenderPassSpecification extCompRenderPassSpec;
			extCompRenderPassSpec.TargetFramebuffer = std::make_shared<VulkanFramebuffer>(extCompFramebufferSpec);

			PipelineSpecification extCompPipelineSpec;
			extCompPipelineSpec.DebugName = "External Composite Pipeline";
			extCompPipelineSpec.pShader = Renderer::GetShader("ExtComposite");
			extCompPipelineSpec.pRenderPass = std::make_shared<VulkanRenderPass>(extCompRenderPassSpec);
			extCompPipelineSpec.DepthTest = false;
			extCompPipelineSpec.DepthWrite = false;

			m_ExternalCompositePipeline = std::make_shared<VulkanPipeline>(extCompPipelineSpec);
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
			skyboxPipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" }
			};

			skyboxPipelineSpec.pRenderPass = m_GeometryPipeline->GetSpecification().pRenderPass;

			m_SkyboxPipeline = std::make_shared<VulkanPipeline>(skyboxPipelineSpec);
		}

		// DOF Pipeline
		{
			m_DOFPipeline = std::make_shared<VulkanComputePipeline>(Renderer::GetShader("DOF"), "Depth of Field Pipeline");
		}

		// Bloom Pipeline
		{
			m_BloomPipeline = std::make_shared<VulkanComputePipeline>(Renderer::GetShader("Bloom"), "Bloom Pipeline");
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
			m_GeometryMaterial->SetTexture(6, m_IrradianceTexture);
			m_GeometryMaterial->SetImage(7, m_BRDFTexture);
			m_GeometryMaterial->SetTexture(8, m_PrefilteredTexture);
			m_GeometryMaterial->PrepareShaderMaterial();
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
			m_BloomPrefilterShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Prefilter Shader Material");

			m_BloomPrefilterShaderMaterial->SetImage(0, m_BloomTextures[0]);
			m_BloomPrefilterShaderMaterial->SetImages(1, m_SceneRenderTextures);
			m_BloomPrefilterShaderMaterial->SetImages(2, m_SceneRenderTextures);
			m_BloomPrefilterShaderMaterial->PrepareShaderMaterial();

			m_BloomPingShaderMaterials.resize(mipCount - 1);
			m_BloomPongShaderMaterials.resize(mipCount - 1);
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

				m_BloomPingShaderMaterials[i - 1] = bloomPingShaderMaterial;

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

				m_BloomPongShaderMaterials[i - 1] = bloomPongShaderMaterial;
			}

			// Set D: First Upsampling
			// Binding 0(o_Image): BloomTex[2]
			// Binding 1(u_Texture): BloomTex[0]
			// Binding 2(u_BloomTexture): RenderTex
			m_BloomUpsampleFirstShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Upsample First Shader Material");

			bloomTexture2->CreateImageViewSingleMip(mipCount - 1);
			m_BloomUpsampleFirstShaderMaterial->SetImage(0, m_BloomTextures[2], mipCount - 1);
			m_BloomUpsampleFirstShaderMaterial->SetImage(1, m_BloomTextures[0]);
			m_BloomUpsampleFirstShaderMaterial->SetImages(2, m_SceneRenderTextures);
			m_BloomUpsampleFirstShaderMaterial->PrepareShaderMaterial();

			// Set E: Final Upsampling
			// Binding 0(o_Image): BloomTex[2]
			// Binding 1(u_Texture): BloomTex[0]
			// Binding 2(u_BloomTexture): BloomTex[2]
			m_BloomUpsampleShaderMaterials.resize(mipCount - 1);
			for (int i = mipCount - 2; i >= 0; --i)
			{
				auto bloomUpsampleShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Upsample Shader Material");

				bloomTexture2->CreateImageViewSingleMip(i);
				bloomUpsampleShaderMaterial->SetImage(0, m_BloomTextures[2], i);
				bloomUpsampleShaderMaterial->SetImage(1, m_BloomTextures[0]);
				bloomUpsampleShaderMaterial->SetImage(2, m_BloomTextures[2]);
				bloomUpsampleShaderMaterial->PrepareShaderMaterial();

				m_BloomUpsampleShaderMaterials[i] = bloomUpsampleShaderMaterial;
			}
		}

		// External Composite Material
		{
			m_ExternalCompositeShaderMaterial = std::make_shared<VulkanMaterial>(m_ExternalCompositePipeline->GetSpecification().pShader, "External Composite Shader Material");

			auto geomFB = std::static_pointer_cast<VulkanFramebuffer>(m_GeometryPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer);
			m_ExternalCompositeShaderMaterial->SetImages(1, geomFB->GetAttachment(true));
			m_ExternalCompositeShaderMaterial->SetImage(2, m_BloomTextures[2]);
			m_ExternalCompositeShaderMaterial->SetTexture(3, std::dynamic_pointer_cast<VulkanTexture>(m_BloomDirtTexture));
			m_ExternalCompositeShaderMaterial->PrepareShaderMaterial();
		}

		// DOF Material
		{
			m_DOFMaterial = std::make_shared<VulkanMaterial>(m_DOFPipeline->GetShader(), "DOF Shader Material");

			auto geomFB = std::static_pointer_cast<VulkanFramebuffer>(m_GeometryPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer);
			auto extCompFB = std::static_pointer_cast<VulkanFramebuffer>(m_ExternalCompositePipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer);
			m_DOFMaterial->SetImages(0, m_DOFComputeTextures);
			m_DOFMaterial->SetBuffers(1, m_UBCamera);
			m_DOFMaterial->SetImages(2, extCompFB->GetAttachment(true));
			m_DOFMaterial->SetImages(3, geomFB->GetDepthAttachment(true));
			m_DOFMaterial->SetBuffers(4, m_UBDOFData);
			m_DOFMaterial->PrepareShaderMaterial();
		}

		// Composite Material
		{
			m_CompositeShaderMaterial = std::make_shared<VulkanMaterial>(m_CompositePipeline->GetSpecification().pShader, "Composite Shader Material");

			m_CompositeShaderMaterial->SetImages(0, m_DOFComputeTextures);
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
			m_BloomPrefilterShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Prefilter Shader Material");

			m_BloomPrefilterShaderMaterial->SetImage(0, m_BloomTextures[0]);
			m_BloomPrefilterShaderMaterial->SetImages(1, m_SceneRenderTextures);
			m_BloomPrefilterShaderMaterial->SetImages(2, m_SceneRenderTextures);
			m_BloomPrefilterShaderMaterial->PrepareShaderMaterial();

			m_BloomPingShaderMaterials.resize(mipCount - 1);
			m_BloomPongShaderMaterials.resize(mipCount - 1);
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

				m_BloomPingShaderMaterials[i - 1] = bloomPingShaderMaterial;

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

				m_BloomPongShaderMaterials[i - 1] = bloomPongShaderMaterial;
			}

			// Set D: First Upsampling
			// Binding 0(o_Image): BloomTex[2]
			// Binding 1(u_Texture): BloomTex[0]
			// Binding 2(u_BloomTexture): RenderTex
			m_BloomUpsampleFirstShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Upsample First Shader Material");

			bloomTexture2->CreateImageViewSingleMip(mipCount - 1);
			m_BloomUpsampleFirstShaderMaterial->SetImage(0, m_BloomTextures[2], mipCount - 1);
			m_BloomUpsampleFirstShaderMaterial->SetImage(1, m_BloomTextures[0]);
			m_BloomUpsampleFirstShaderMaterial->SetImages(2, m_SceneRenderTextures);
			m_BloomUpsampleFirstShaderMaterial->PrepareShaderMaterial();

			// Set E: Final Upsampling
			// Binding 0(o_Image): BloomTex[2]
			// Binding 1(u_Texture): BloomTex[0]
			// Binding 2(u_BloomTexture): BloomTex[2]
			m_BloomUpsampleShaderMaterials.resize(mipCount - 1);
			for (int i = mipCount - 2; i >= 0; --i)
			{
				auto bloomUpsampleShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Upsample Shader Material");

				bloomTexture2->CreateImageViewSingleMip(i);
				bloomUpsampleShaderMaterial->SetImage(0, m_BloomTextures[2], i);
				bloomUpsampleShaderMaterial->SetImage(1, m_BloomTextures[0]);
				bloomUpsampleShaderMaterial->SetImage(2, m_BloomTextures[2]);
				bloomUpsampleShaderMaterial->PrepareShaderMaterial();

				m_BloomUpsampleShaderMaterials[i] = bloomUpsampleShaderMaterial;
			}
		}

		// External Composite Material
		{
			auto geomFB = std::static_pointer_cast<VulkanFramebuffer>(m_GeometryPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer);

			m_ExternalCompositeShaderMaterial->SetImages(1, geomFB->GetAttachment(true));
			m_ExternalCompositeShaderMaterial->SetImage(2, m_BloomTextures[2]);
			m_ExternalCompositeShaderMaterial->SetTexture(3, std::dynamic_pointer_cast<VulkanTexture>(m_BloomDirtTexture));
			m_ExternalCompositeShaderMaterial->PrepareShaderMaterial();
		}

		// DOF Material
		{
			auto geomFB = std::static_pointer_cast<VulkanFramebuffer>(m_GeometryPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer);
			auto extCompFB = std::static_pointer_cast<VulkanFramebuffer>(m_ExternalCompositePipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer);

			m_DOFMaterial->SetImages(0, m_DOFComputeTextures);
			m_DOFMaterial->SetBuffers(1, m_UBCamera);
			m_DOFMaterial->SetImages(2, extCompFB->GetAttachment(true));
			m_DOFMaterial->SetImages(3, geomFB->GetDepthAttachment(true));
			m_DOFMaterial->SetBuffers(4, m_UBDOFData);
			m_DOFMaterial->PrepareShaderMaterial();
		}

		// Composite Material
		{
			m_CompositeShaderMaterial->SetImages(0, m_DOFComputeTextures);
			m_CompositeShaderMaterial->PrepareShaderMaterial();
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
		m_GeometryPipeline->ReloadPipeline();
		m_LightPipeline->ReloadPipeline();
		m_CompositePipeline->ReloadPipeline();
		m_SkyboxPipeline->ReloadPipeline();
		m_BloomPipeline->ReloadPipeline();
	}

	void SceneRenderer::CreateResources()
	{
		auto device = VulkanContext::GetCurrentDevice();

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		Renderer::WaitAndExecute();

		m_SceneImages.resize(framesInFlight);

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			std::shared_ptr<VulkanImage> finalPassImage = std::dynamic_pointer_cast<VulkanImage>(GetFinalPassImage(i));
			m_SceneImages[i] = ImGuiLayer::AddTexture(*finalPassImage);
		}

		m_UBCamera.reserve(framesInFlight);
		m_UBPointLight.reserve(framesInFlight);
		m_UBSpotLight.reserve(framesInFlight);
		m_UBDOFData.reserve(framesInFlight);

		// Uniform Buffers
		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			m_UBCamera.emplace_back(std::make_shared<VulkanUniformBuffer>(sizeof(UBCamera)));
			m_UBPointLight.emplace_back(std::make_shared<VulkanUniformBuffer>(sizeof(UBPointLights)));
			m_UBSpotLight.emplace_back(std::make_shared<VulkanUniformBuffer>(sizeof(UBSpotLights)));
			m_UBDOFData.emplace_back(std::make_shared<VulkanUniformBuffer>(sizeof(DOFSettings)));
		}

		m_BloomMipSize = (glm::uvec2(m_ViewportSize.x, m_ViewportSize.y) + 1u) / 2u;
		m_BloomMipSize += 16u - m_BloomMipSize % 16u;

		ImageSpecification bloomRTSpec = {};
		bloomRTSpec.DebugName = "Bloom Compute Texture";
		bloomRTSpec.Width = m_BloomMipSize.x;
		bloomRTSpec.Height = m_BloomMipSize.y;
		bloomRTSpec.Format = ImageFormat::RGBA32F;
		bloomRTSpec.Usage = ImageUsage::Storage;
		bloomRTSpec.MipLevels = Utils::CalculateMipCount(m_BloomMipSize.x, m_BloomMipSize.y) - 2;

		m_BloomTextures.reserve(framesInFlight);
		m_SceneRenderTextures.reserve(framesInFlight);

		VkCommandBuffer barrierCmd = device->GetCommandBuffer();

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			auto BloomTexture = std::static_pointer_cast<VulkanImage>(m_BloomTextures.emplace_back(std::make_shared<VulkanImage>(bloomRTSpec)));
			BloomTexture->Invalidate();

			Utils::InsertImageMemoryBarrier(barrierCmd, BloomTexture->GetVulkanImageInfo().Image,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, BloomTexture->GetSpecification().MipLevels, 0, 1 });
		}

		ImageSpecification sceneRTSpec = {};
		sceneRTSpec.DebugName = "Scene Render Texture";
		sceneRTSpec.Width = m_ViewportSize.x;
		sceneRTSpec.Height = m_ViewportSize.y;
		sceneRTSpec.Format = ImageFormat::RGBA32F;
		sceneRTSpec.Usage = ImageUsage::Texture;
		sceneRTSpec.MipLevels = Utils::CalculateMipCount(m_ViewportSize.x, m_ViewportSize.y);

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			auto SceneTexture = std::static_pointer_cast<VulkanImage>(m_SceneRenderTextures.emplace_back(std::make_shared<VulkanImage>(sceneRTSpec)));
			SceneTexture->Invalidate();

			Utils::InsertImageMemoryBarrier(barrierCmd, SceneTexture->GetVulkanImageInfo().Image,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, SceneTexture->GetSpecification().MipLevels, 0, 1 });
		}

		ImageSpecification dofImageSpec;
		dofImageSpec.DebugName = "DOF Output Textures";
		dofImageSpec.Width = m_ViewportSize.x;
		dofImageSpec.Height = m_ViewportSize.y;
		dofImageSpec.Format = ImageFormat::RGBA32F;
		dofImageSpec.Usage = ImageUsage::Storage;
		dofImageSpec.Transfer = true;

		m_DOFComputeTextures.reserve(framesInFlight);

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			auto DOFTexture = std::static_pointer_cast<VulkanImage>(m_DOFComputeTextures.emplace_back(std::make_shared<VulkanImage>(dofImageSpec)));
			DOFTexture->Invalidate();

			Utils::InsertImageMemoryBarrier(barrierCmd, DOFTexture->GetVulkanImageInfo().Image,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, DOFTexture->GetSpecification().MipLevels, 0, 1 });
		}

		device->FlushCommandBuffer(barrierCmd);

		m_BloomDirtTexture = AssetManager::GetAsset<Texture2D>("assets/textures/LensDirt.png");

		auto blackTextureCube = std::dynamic_pointer_cast<VulkanTextureCube>(Renderer::GetBlackTextureCube(ImageFormat::RGBA8_UNORM));
		blackTextureCube->Invalidate();

		m_CubemapTexture = blackTextureCube;
		m_PrefilteredTexture = blackTextureCube;
		m_IrradianceTexture = blackTextureCube;

		m_SkyboxTextureID = ImGuiLayer::AddTexture(*std::dynamic_pointer_cast<VulkanTextureCube>(m_PrefilteredTexture));

		m_BRDFTexture = Renderer::CreateBRDFTexture();
		m_PointLightTextureIcon = TextureImporter::LoadTexture2D("../EditorLayer/Resources/Icons/PointLightIcon.png");
		m_SpotLightTextureIcon = TextureImporter::LoadTexture2D("../EditorLayer/Resources/Icons/SpotLightIcon.png");

		m_SkyboxVBData = Utils::CreateCubeModel();
	}

	void SceneRenderer::Release()
	{
	}

	void SceneRenderer::RecreateScene()
	{
		auto device = VulkanContext::GetCurrentDevice();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		VK_CORE_INFO("Scene has been Recreated!");
		m_GeometryPipeline->GetSpecification().pRenderPass->RecreateFramebuffers(m_ViewportSize.x, m_ViewportSize.y);
		m_CompositePipeline->GetSpecification().pRenderPass->RecreateFramebuffers(m_ViewportSize.x, m_ViewportSize.y);
		m_ExternalCompositePipeline->GetSpecification().pRenderPass->RecreateFramebuffers(m_ViewportSize.x, m_ViewportSize.y);

		// Recreate Resources
		{
			m_BloomMipSize = (glm::uvec2(m_ViewportSize.x, m_ViewportSize.y) + 1u) / 2u;
			m_BloomMipSize += 16u - m_BloomMipSize % 16u;

			VkCommandBuffer barrierCmd = device->GetCommandBuffer();

			for (auto& BloomTexture : m_BloomTextures)
			{
				auto vulkanBloomTexture = std::dynamic_pointer_cast<VulkanImage>(BloomTexture);
				vulkanBloomTexture->Resize(m_BloomMipSize.x, m_BloomMipSize.y, Utils::CalculateMipCount(m_BloomMipSize.x, m_BloomMipSize.y) - 2);

				Utils::InsertImageMemoryBarrier(barrierCmd, vulkanBloomTexture->GetVulkanImageInfo().Image,
					VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, BloomTexture->GetSpecification().MipLevels, 0, 1 });
			}

			for (auto& SceneRenderTexture : m_SceneRenderTextures)
			{
				auto vulkanSceneTexture = std::dynamic_pointer_cast<VulkanImage>(SceneRenderTexture);
				vulkanSceneTexture->Resize(m_ViewportSize.x, m_ViewportSize.y, Utils::CalculateMipCount(m_ViewportSize.x, m_ViewportSize.y));

				Utils::InsertImageMemoryBarrier(barrierCmd, vulkanSceneTexture->GetVulkanImageInfo().Image,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, SceneRenderTexture->GetSpecification().MipLevels, 0, 1 });
			}

			for (auto& DOFTexture : m_DOFComputeTextures)
			{
				auto vulkanDOFTexture = std::dynamic_pointer_cast<VulkanImage>(DOFTexture);
				vulkanDOFTexture->Resize(m_ViewportSize.x, m_ViewportSize.y);

				Utils::InsertImageMemoryBarrier(barrierCmd, vulkanDOFTexture->GetVulkanImageInfo().Image,
					VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, DOFTexture->GetSpecification().MipLevels, 0, 1 });
			}

			device->FlushCommandBuffer(barrierCmd);

			// Update ImGui Viewport Image
			for (uint32_t i = 0; i < framesInFlight; ++i)
			{
				std::shared_ptr<VulkanImage> finalPassImage = std::dynamic_pointer_cast<VulkanImage>(GetFinalPassImage(i));
				ImGuiLayer::UpdateDescriptor(m_SceneImages[i], *finalPassImage);
			}
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
			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Depth of Field Seetings", treeFlags))
		{
			ImGui::DragFloat("Focus Point", &m_DOFSettings.FocusPoint, 0.01f, 0.0f, 50.0f);
			ImGui::DragFloat("Focus Scale", &m_DOFSettings.FocusScale, 0.01f, 0.0f, 50.0f);
			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Scene Renderer Stats##GPUPerf", treeFlags))
		{
			auto vulkanCmdBuffer = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer);

			ImGui::Text("Geometry Pass: %lluns", vulkanCmdBuffer->GetQueryTime(1));
			ImGui::Text("Skybox Pass: %lluns", vulkanCmdBuffer->GetQueryTime(0));
			ImGui::Text("Light Pass: %lluns", vulkanCmdBuffer->GetQueryTime(2));
			ImGui::Text("Composite Pass: %lluns", vulkanCmdBuffer->GetQueryTime(6));
			ImGui::Text("Bloom Compute Pass: %lluns", vulkanCmdBuffer->GetQueryTime(3));
			ImGui::Text("Depth of Field Compute Pass: %lluns", vulkanCmdBuffer->GetQueryTime(5));
			ImGui::Text("External Composite Pass: %lluns", vulkanCmdBuffer->GetQueryTime(4));
			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Bloom Settings", treeFlags))
		{
			ImGui::DragFloat("Threshold", &m_BloomParams.Threshold, 0.01f, 0.0f, 1000.0f);
			ImGui::DragFloat("Knee", &m_BloomParams.Knee, 0.01f, 0.001f, 1.0f);
			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Scene Shader Map", treeFlags))
		{
			auto& shaderMap = Renderer::GetShaderMap();

			int buttonID = 0;
			for (auto&& [name, shader] : shaderMap)
			{
				ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_AllowItemOverlap, ImVec2{ 0.0f, 24.5f });
				ImGui::SetItemAllowOverlap();

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

		if (ImGui::TreeNodeEx("Scene Draw Stats", treeFlags))
		{
			auto renderStats = VulkanRenderer::GetRendererStats();

			ImGui::Text("Draw Calls: %u", renderStats.DrawCalls);
			ImGui::Text("Instance Count: %u", renderStats.InstanceCount);
			ImGui::Text("Draw Calls Saved: %u", renderStats.InstanceCount - renderStats.DrawCalls);
			ImGui::TreePop();
		}

		ImGui::End(); // End of Scene Renderer Window

		VulkanRenderer::ResetStats();
	}

	void SceneRenderer::SetActiveScene(std::shared_ptr<Scene> scene)
	{
		m_Scene = scene;

		m_MeshDrawList.clear();
		m_MeshTransformMap.clear();
	}

	void SceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
	{
		m_ViewportSize.x = width;
		m_ViewportSize.y = height;

		RecreateScene();
	}

	void SceneRenderer::RenderScene(EditorCamera& camera)
	{
		VK_CORE_PROFILE_FN("Submit-SceneRenderer");

		int frameIndex = Renderer::GetCurrentFrameIndex();

		// Camera
		UBCamera cameraUB{};
		cameraUB.Projection = camera.GetProjectionMatrix();
		cameraUB.View = camera.GetViewMatrix();
		cameraUB.InverseView = glm::inverse(camera.GetViewMatrix());

		float nearClip = camera.GetNearClip();
		float farClip = camera.GetFarClip();
		cameraUB.DepthUnpackConsts.x = (farClip * nearClip) / (farClip - nearClip);
		cameraUB.DepthUnpackConsts.y = (farClip + nearClip) / (farClip - nearClip);

		float fovy = camera.GetFieldOfViewY();
		float aspectRatio = camera.GetAspectRatio();
		float tanHalfFOVy = glm::tan(fovy / 2.0f);
		float tanHalfFOVx = tanHalfFOVy * aspectRatio;

		cameraUB.CameraTanHalfFOV = { tanHalfFOVx, tanHalfFOVy };
		m_UBCamera[frameIndex]->WriteAndFlushBuffer(&cameraUB);

		//VK_CORE_WARN("Size of Camera: {}, Byte Offset of CameraTanHalfFOV: {}", sizeof(UBCamera), offsetof(UBCamera, CameraTanHalfFOV));

		// Point Light
		UBPointLights pointLightUB{};
		m_Scene->UpdatePointLightUB(pointLightUB);
		m_UBPointLight[frameIndex]->WriteAndFlushBuffer(&pointLightUB);

		UBSpotLights spotLightUB{};
		m_Scene->UpdateSpotLightUB(spotLightUB);
		m_UBSpotLight[frameIndex]->WriteAndFlushBuffer(&spotLightUB);
		
		// DOF Settings
		m_UBDOFData[frameIndex]->WriteAndFlushBuffer(&m_DOFSettings);

		m_SceneCommandBuffer->Begin();

		GeometryPass();
		BloomCompute();
		ExternalCompositePass();
		DOFCompute();
		CompositePass();

		ResetDrawCommands();

		m_SceneCommandBuffer->End();
	}

	void SceneRenderer::RenderLights()
	{ 
		Renderer::Submit([this]
		{
			VkCommandBuffer bindCmd = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer)->RT_GetActiveCommandBuffer();
			auto vulkanLightPipeline = std::static_pointer_cast<VulkanPipeline>(m_LightPipeline);
			vulkanLightPipeline->Bind(bindCmd);

			// Binding Point Light Descriptor Set
			m_PointLightShaderMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_LightPipeline);
		});

		// Point Lights
		for (auto pointLightPosition : m_PointLightPositions)
		{
			Renderer::Submit([this, pointLightPosition]
			{
				VK_CORE_PROFILE_FN("Render-PointLights");

				VkCommandBuffer drawCmd = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer)->RT_GetActiveCommandBuffer();

				auto vulkanLightPipeline = std::static_pointer_cast<VulkanPipeline>(m_LightPipeline);
				vulkanLightPipeline->SetPushConstants(drawCmd, (void*)&pointLightPosition, sizeof(glm::vec4));
				vkCmdDraw(drawCmd, 6, 1, 0, 0);
			});
		}

		Renderer::Submit([this] { m_SpotLightShaderMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_LightPipeline); });

		// Spot Lights
		for (auto spotLightPosition : m_SpotLightPositions)
		{
			Renderer::Submit([this, spotLightPosition]
			{
				VK_CORE_PROFILE_FN("Render-SpotLights");

				VkCommandBuffer drawCmd = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer)->RT_GetActiveCommandBuffer();

				auto vulkanLightPipeline = std::static_pointer_cast<VulkanPipeline>(m_LightPipeline);
				vulkanLightPipeline->SetPushConstants(drawCmd, (void*)&spotLightPosition, sizeof(glm::vec4));
				vkCmdDraw(drawCmd, 6, 1, 0, 0);
			});
		}
	}

	void SceneRenderer::SubmitMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const glm::mat4& transform)
	{
		VK_CORE_PROFILE();

		if (!mesh || !materialAsset)
			return;

		auto meshSource = mesh->GetMeshSource();
		auto& submeshData = meshSource->GetSubmeshes();
		uint64_t meshHandle = mesh->Handle;
		uint64_t materialHandle = materialAsset->Handle;

		if (meshSource->GetVertexCount() == 0)
			return;

		for (uint32_t submeshIndex : mesh->GetSubmeshes())
		{
			MeshKey meshKey = { meshHandle, materialHandle, submeshIndex };
			auto& transformBuffer = m_MeshTransformMap[meshKey].Transforms.emplace_back();

			glm::mat4 submeshTransform = transform * submeshData[submeshIndex].LocalTransform;
			transformBuffer.MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			transformBuffer.MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			transformBuffer.MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };

			auto& dc = m_MeshDrawList[meshKey];
			dc.MeshInstance = mesh;
			dc.MaterialInstance = materialAsset->GetMaterial();
			dc.SubmeshIndex = submeshIndex;
			dc.TransformBuffer = m_MeshTransformMap[meshKey].TransformBuffer;
			dc.InstanceCount++;
		}
	}

	void SceneRenderer::SubmitTransparentMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const glm::mat4& transform)
	{

	}

	void SceneRenderer::UpdateMeshInstanceData(std::shared_ptr<Mesh> mesh, std::shared_ptr<MaterialAsset> materialAsset)
	{
		uint64_t meshHandle = mesh->Handle;
		uint64_t materialHandle = materialAsset->Handle;

		for (uint32_t submeshIndex : mesh->GetSubmeshes())
		{
			MeshKey meshKey = { meshHandle, materialHandle, submeshIndex };
			m_MeshTransformMap.erase(meshKey);
			m_MeshDrawList.erase(meshKey);
		}
	}

	std::shared_ptr<Image2D> SceneRenderer::GetFinalPassImage(uint32_t index) const
	{
		auto framebuffer = std::dynamic_pointer_cast<VulkanFramebuffer>(m_SceneFramebuffer);
		return framebuffer->GetAttachment(true)[index];
	}
		
	void SceneRenderer::UpdateSkybox(const std::string& filepath)
	{
		// Obtain Cubemaps
		auto [filteredMap, irradianceMap] = VulkanRenderer::CreateEnviromentMap(filepath);
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

	void SceneRenderer::SetSkybox(const std::string& filepath)
	{
		s_Instance->UpdateSkybox(filepath);
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

	void SceneRenderer::GeometryPass()
	{
		m_Scene->OnUpdateGeometry(this);
		m_Scene->OnUpdateLights(m_PointLightPositions, m_SpotLightPositions);

		Renderer::BeginRenderPass(m_SceneCommandBuffer, m_GeometryPipeline->GetSpecification().pRenderPass);

		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Skybox", DebugLabelColor::Orange);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		// Rendering Skybox
		Renderer::RenderSkybox(m_SceneCommandBuffer, m_SkyboxPipeline, m_SkyboxVBData, m_SkyboxMaterial, &m_SkyboxSettings);
		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);

		// Rendering Geometry
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Geometry-Pass", DebugLabelColor::Gold);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		Renderer::Submit([this]
		{
			VkCommandBuffer bindCmd = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer)->RT_GetActiveCommandBuffer();
			auto vulkanGeometryPipeline = std::static_pointer_cast<VulkanPipeline>(m_GeometryPipeline);
			vulkanGeometryPipeline->Bind(bindCmd);

			// Binding Static Geometry Descriptor Sets
			m_GeometryMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_GeometryPipeline);
		});

		for (auto& [mk, dc] : m_MeshDrawList)
			Renderer::RenderMesh(m_SceneCommandBuffer, dc.MeshInstance, dc.MaterialInstance, dc.SubmeshIndex, m_GeometryPipeline, dc.TransformBuffer, m_MeshTransformMap[mk].Transforms, dc.InstanceCount);

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
			m_GeometryPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer->GetAttachment(true)[frameIndex],
			m_SceneRenderTextures[frameIndex]);

		Renderer::BlitVulkanImage(m_SceneCommandBuffer, m_SceneRenderTextures[frameIndex]);

		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
	}


	void SceneRenderer::ExternalCompositePass()
	{
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "External-Composite-Pass", DebugLabelColor::Aqua);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		Renderer::BeginRenderPass(m_SceneCommandBuffer, m_ExternalCompositePipeline->GetSpecification().pRenderPass);

		Renderer::Submit([this]
		{
			auto vulkanCmdBuffer = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer);
			auto vulkanExtCompPipeline = std::static_pointer_cast<VulkanPipeline>(m_ExternalCompositePipeline);

			vulkanExtCompPipeline->SetPushConstants(vulkanCmdBuffer->RT_GetActiveCommandBuffer(), &m_SceneSettings, sizeof(SceneSettings));
		});

		Renderer::SubmitFullscreenQuad(m_SceneCommandBuffer, m_ExternalCompositePipeline, m_ExternalCompositeShaderMaterial);
		Renderer::EndRenderPass(m_SceneCommandBuffer, m_ExternalCompositePipeline->GetSpecification().pRenderPass);

		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
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

			m_BloomPrefilterShaderMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);

			const uint32_t mips = m_BloomTextures[0]->GetSpecification().MipLevels;
			glm::uvec2 bloomMipSize = m_BloomMipSize;
			glm::uvec2 bloomWorkGroups = glm::max(bloomMipSize / 16u, 1u);

			vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
			vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_BloomParams, sizeof(glm::vec2), sizeof(glm::vec2));
			vulkanBloomPipeline->Dispatch(dispatchCmd, bloomWorkGroups.x, bloomWorkGroups.y, 1);

			for (uint32_t i = 1; i < mips; i++)
			{
				m_LodAndMode.LOD = float(i - 1);
				m_LodAndMode.Mode = 1.0f;

				int currentIdx = i - 1;

				m_BloomPingShaderMaterials[currentIdx]->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);
				bloomMipSize = m_BloomTextures[0]->GetMipSize(i);
				bloomWorkGroups = glm::max(bloomMipSize / 16u, 1u);

				vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
				vulkanBloomPipeline->Dispatch(dispatchCmd, bloomWorkGroups.x, bloomWorkGroups.y, 1);

				m_LodAndMode.LOD = (float)i;
				
				m_BloomPongShaderMaterials[currentIdx]->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);

				vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
				vulkanBloomPipeline->Dispatch(dispatchCmd, bloomWorkGroups.x, bloomWorkGroups.y, 1);
			}

			// Upsample First
			// TODO: Could have to use VkImageSubresourceRange to set correct mip level
			m_LodAndMode.LOD = float(mips - 2);
			m_LodAndMode.Mode = 2.0f;

			m_BloomUpsampleFirstShaderMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);

			bloomMipSize = m_BloomTextures[2]->GetMipSize(mips - 1);
			bloomWorkGroups = glm::max(bloomMipSize / 16u, 1u);

			vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
			vulkanBloomPipeline->Dispatch(dispatchCmd, bloomWorkGroups.x, bloomWorkGroups.y, 1);

			// Upsample Final
			for (int i = mips - 2; i >= 0; --i)
			{
				m_LodAndMode.LOD = (float)i;
				m_LodAndMode.Mode = 3.0f;

				m_BloomUpsampleShaderMaterials[i]->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);

				bloomMipSize = m_BloomTextures[2]->GetMipSize(i);
				bloomWorkGroups = glm::max(bloomMipSize / 16u, 1u);

				vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
				vulkanBloomPipeline->Dispatch(dispatchCmd, bloomWorkGroups.x, bloomWorkGroups.y, 1);
			}
		});

		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
	}

	void SceneRenderer::DOFCompute()
	{
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "DOF", DebugLabelColor::Green);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		Renderer::Submit([this]
		{
			auto vulkanDOFPipeline = std::static_pointer_cast<VulkanComputePipeline>(m_DOFPipeline);
			auto vulkanDOFMaterial = std::static_pointer_cast<VulkanMaterial>(m_DOFMaterial);

			VkCommandBuffer dispatchCmd = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer)->RT_GetActiveCommandBuffer();
			vulkanDOFPipeline->Bind(dispatchCmd);

			int frameIndex = Renderer::RT_GetCurrentFrameIndex();
			auto dofTexture = std::static_pointer_cast<VulkanImage>(m_DOFComputeTextures[frameIndex]);

			VkImageSubresourceRange subresourceRange{};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;

			VkClearColorValue clearColorValue{ { 0.0f, 0.0f, 0.0f, 1.0f } };
			vkCmdClearColorImage(dispatchCmd, dofTexture->GetVulkanImageInfo().Image, VK_IMAGE_LAYOUT_GENERAL, &clearColorValue, 1, &subresourceRange);

			glm::uvec2 dofImageSize = { dofTexture->GetSpecification().Width, dofTexture->GetSpecification().Height };

			vkCmdBindDescriptorSets(dispatchCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
				vulkanDOFPipeline->GetVulkanPipelineLayout(), 0, 1,
				&vulkanDOFMaterial->RT_GetVulkanMaterialDescriptorSet(), 0, nullptr);

			vulkanDOFPipeline->Dispatch(dispatchCmd, dofImageSize.x / 16, dofImageSize.y / 16, 1);
		});

		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
	}

	void SceneRenderer::CreateCommandBuffers()
	{
		auto device = VulkanContext::GetCurrentDevice();
		m_SceneCommandBuffer = std::make_shared<VulkanRenderCommandBuffer>(device->GetCommandPool(), CommandBufferLevel::Primary, 7);
	}

	void SceneRenderer::ResetDrawCommands()
	{
		for (auto& [mk, dc] : m_MeshDrawList)
		{
			m_MeshTransformMap[mk].Transforms.clear();
			dc.InstanceCount = 0;
		}

		m_PointLightPositions.clear();
		m_SpotLightPositions.clear();
	}

}