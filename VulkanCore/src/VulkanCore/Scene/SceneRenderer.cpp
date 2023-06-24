#include "vulkanpch.h"
#include "SceneRenderer.h"

#include "VulkanCore/Asset/AssetManager.h"
#include "VulkanCore/Asset/TextureImporter.h"
#include "VulkanCore/Asset/MaterialAsset.h"
#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Core/Timer.h"
#include "VulkanCore/Renderer/Renderer.h"

#include "VulkanCore/Mesh/Mesh.h"
#include "Platform/Vulkan/VulkanAllocator.h"
#include "Platform/Vulkan/VulkanMaterial.h"

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
			geomFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::DEPTH24STENCIL8 };
			geomFramebufferSpec.Transfer = true;
			geomFramebufferSpec.Samples = 8;

			RenderPassSpecification geomRenderPassSpec;
			geomRenderPassSpec.TargetFramebuffer = std::make_shared<VulkanFramebuffer>(geomFramebufferSpec);

			PipelineSpecification geomPipelineSpec;
			geomPipelineSpec.DebugName = "Geometry Pipeline";
			geomPipelineSpec.pShader = Renderer::GetShader("CorePBR");
			geomPipelineSpec.RenderPass = std::make_shared<VulkanRenderPass>(geomRenderPassSpec);
			geomPipelineSpec.Layout = vertexLayout;
			geomPipelineSpec.InstanceLayout = instanceLayout;

			PipelineSpecification lightPipelineSpec;
			lightPipelineSpec.DebugName = "Light Pipeline";
			lightPipelineSpec.pShader = Renderer::GetShader("LightShader");
			lightPipelineSpec.Blend = true;
			lightPipelineSpec.RenderPass = geomPipelineSpec.RenderPass;

			m_GeometryPipeline = std::make_shared<VulkanPipeline>(geomPipelineSpec);
			m_LightPipeline = std::make_shared<VulkanPipeline>(lightPipelineSpec);
		}

		// Composite Pipeline
		{
			FramebufferSpecification compFramebufferSpec;
			compFramebufferSpec.Width = 1920;
			compFramebufferSpec.Height = 1080;
			compFramebufferSpec.Attachments = { ImageFormat::RGBA8_SRGB };
			compFramebufferSpec.Samples = 8;

			RenderPassSpecification compRenderPassSpec;
			compRenderPassSpec.TargetFramebuffer = std::make_shared<VulkanFramebuffer>(compFramebufferSpec);
			m_SceneFramebuffer = compRenderPassSpec.TargetFramebuffer;

			PipelineSpecification compPipelineSpec;
			compPipelineSpec.DebugName = "Composite Pipeline";
			compPipelineSpec.pShader = Renderer::GetShader("SceneComposite");
			compPipelineSpec.RenderPass = std::make_shared<VulkanRenderPass>(compRenderPassSpec);
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

			skyboxPipelineSpec.RenderPass = m_GeometryPipeline->GetSpecification().RenderPass;

			m_SkyboxPipeline = std::make_shared<VulkanPipeline>(skyboxPipelineSpec);
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

			// Set A : Prefiltering
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

				m_BloomTextures[1]->CreateImageViewSingleMip(i);
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
				
				m_BloomTextures[0]->CreateImageViewSingleMip(i);
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

			m_BloomTextures[2]->CreateImageViewSingleMip(mipCount - 1);
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

				m_BloomTextures[2]->CreateImageViewSingleMip(i);
				bloomUpsampleShaderMaterial->SetImage(0, m_BloomTextures[2], i);
				bloomUpsampleShaderMaterial->SetImage(1, m_BloomTextures[0]);
				bloomUpsampleShaderMaterial->SetImage(2, m_BloomTextures[2]);
				bloomUpsampleShaderMaterial->PrepareShaderMaterial();

				m_BloomUpsampleShaderMaterials[i] = bloomUpsampleShaderMaterial;
			}
		}

		// Composite Material
		{
			m_CompositeShaderMaterial = std::make_shared<VulkanMaterial>(m_CompositePipeline->GetSpecification().pShader, "Composite Shader Material");

			m_CompositeShaderMaterial->SetImages(0, m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetAttachment(true));
			m_CompositeShaderMaterial->SetImage(1, m_BloomTextures[2]);
			m_CompositeShaderMaterial->SetTexture(2, std::dynamic_pointer_cast<VulkanTexture>(m_BloomDirtTexture));
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

			// Set A : Prefiltering
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

				m_BloomTextures[1]->CreateImageViewSingleMip(i);
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

				m_BloomTextures[0]->CreateImageViewSingleMip(i);
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

			m_BloomTextures[2]->CreateImageViewSingleMip(mipCount - 1);
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

				m_BloomTextures[2]->CreateImageViewSingleMip(i);
				bloomUpsampleShaderMaterial->SetImage(0, m_BloomTextures[2], i);
				bloomUpsampleShaderMaterial->SetImage(1, m_BloomTextures[0]);
				bloomUpsampleShaderMaterial->SetImage(2, m_BloomTextures[2]);
				bloomUpsampleShaderMaterial->PrepareShaderMaterial();

				m_BloomUpsampleShaderMaterials[i] = bloomUpsampleShaderMaterial;
			}
		}

		// Composite Material
		{
			m_CompositeShaderMaterial->SetImages(0, m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetAttachment(true));
			m_CompositeShaderMaterial->SetImage(1, m_BloomTextures[2]);
			m_CompositeShaderMaterial->SetTexture(2, std::dynamic_pointer_cast<VulkanTexture>(m_BloomDirtTexture));
			m_CompositeShaderMaterial->PrepareShaderMaterial();
		}

		// Skybox Material
		{
			m_SkyboxMaterial->SetBuffers(0, m_UBCamera);
			m_SkyboxMaterial->SetTexture(1, m_CubemapTexture);
			m_SkyboxMaterial->PrepareShaderMaterial();
		}
	}

	void SceneRenderer::CreateResources()
	{
		auto device = VulkanContext::GetCurrentDevice();

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		Renderer::WaitAndExecute();

		m_SceneImages.resize(framesInFlight);

		for (int i = 0; i < framesInFlight; i++)
			m_SceneImages[i] = ImGuiLayer::AddTexture(*GetFinalPassImage(i));

		m_UBCamera.reserve(framesInFlight);
		m_UBPointLight.reserve(framesInFlight);
		m_UBSpotLight.reserve(framesInFlight);

		// Uniform Buffers
		for (int i = 0; i < framesInFlight; ++i)
		{
			m_UBCamera.emplace_back(sizeof(UBCamera));
			m_UBPointLight.emplace_back(sizeof(UBPointLights));
			m_UBSpotLight.emplace_back(sizeof(UBSpotLights));
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

		for (int i = 0; i < framesInFlight; i++)
		{
			auto BloomTexture = m_BloomTextures.emplace_back(std::make_shared<VulkanImage>(bloomRTSpec));
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

		for (int i = 0; i < framesInFlight; i++)
		{
			auto SceneTexture = m_SceneRenderTextures.emplace_back(std::make_shared<VulkanImage>(sceneRTSpec));
			SceneTexture->Invalidate();

			Utils::InsertImageMemoryBarrier(barrierCmd, SceneTexture->GetVulkanImageInfo().Image,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, SceneTexture->GetSpecification().MipLevels, 0, 1 });
		}

		device->FlushCommandBuffer(barrierCmd);

		m_BloomDirtTexture = AssetManager::GetAsset<Texture2D>("assets/textures/LensDirt.png");

		auto [filteredMap, irradianceMap] = VulkanRenderer::CreateEnviromentMap("assets/cubemaps/HDR/Birchwood4K.hdr");
		m_CubemapTexture = filteredMap;
		m_PrefilteredTexture = filteredMap;
		m_IrradianceTexture = irradianceMap;

		m_BRDFTexture = VulkanRenderer::CreateBRDFTexture();
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
		m_GeometryPipeline->GetSpecification().RenderPass->RecreateFramebuffers(m_ViewportSize.x, m_ViewportSize.y);
		m_CompositePipeline->GetSpecification().RenderPass->RecreateFramebuffers(m_ViewportSize.x, m_ViewportSize.y);

		// Recreate Resources
		{
			m_BloomMipSize = (glm::uvec2(m_ViewportSize.x, m_ViewportSize.y) + 1u) / 2u;
			m_BloomMipSize += 16u - m_BloomMipSize % 16u;

			VkCommandBuffer barrierCmd = device->GetCommandBuffer();

			for (auto& BloomTexture : m_BloomTextures)
			{
				BloomTexture->Resize(m_BloomMipSize.x, m_BloomMipSize.y, Utils::CalculateMipCount(m_BloomMipSize.x, m_BloomMipSize.y) - 2);

				Utils::InsertImageMemoryBarrier(barrierCmd, BloomTexture->GetVulkanImageInfo().Image,
					VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, BloomTexture->GetSpecification().MipLevels, 0, 1 });
			}

			for (auto& SceneRenderTexture : m_SceneRenderTextures)
			{
				SceneRenderTexture->Resize(m_ViewportSize.x, m_ViewportSize.y, Utils::CalculateMipCount(m_ViewportSize.x, m_ViewportSize.y));

				Utils::InsertImageMemoryBarrier(barrierCmd, SceneRenderTexture->GetVulkanImageInfo().Image,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, SceneRenderTexture->GetSpecification().MipLevels, 0, 1 });
			}

			device->FlushCommandBuffer(barrierCmd);

			// Update ImGui Viewport Image
			for (uint32_t i = 0; i < framesInFlight; ++i)
				ImGuiLayer::UpdateDescriptor(m_SceneImages[i], *GetFinalPassImage(i));
		}

		// Recreate Shader Materials
		RecreateMaterials();
	}

	void SceneRenderer::OnImGuiRender()
	{
		ImGui::Begin("Scene Renderer");

		ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (ImGui::TreeNodeEx("Scene Settings", treeFlags))
		{
			ImGui::DragFloat("Exposure Intensity", &m_SceneSettings.Exposure, 0.01f, 0.0f, 20.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragFloat("Dirt Intensity", &m_SceneSettings.DirtIntensity, 0.01f, 0.0f, 100.0f);
			ImGui::DragFloat("Skybox LOD", &m_SkyboxSettings.LOD, 0.01f, 0.0f, 11.0f);
			ImGui::DragFloat("Skybox Intensity", &m_SkyboxSettings.Intensity, 0.01f, 0.0f, 20.0f);

			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Scene Renderer Stats##GPUPerf", treeFlags))
		{
			m_SceneCommandBuffer->RetrieveQueryPoolResults();

			ImGui::Text("Geometry Pass: %lluns", m_SceneCommandBuffer->GetQueryTime(0));
			ImGui::Text("Skybox Pass: %lluns", m_SceneCommandBuffer->GetQueryTime(1));
			ImGui::Text("Composite Pass: %lluns", m_SceneCommandBuffer->GetQueryTime(4));
			ImGui::Text("Bloom Compute Pass: %lluns", m_SceneCommandBuffer->GetQueryTime(3));
			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Bloom Settings", treeFlags))
		{
			ImGui::DragFloat("Threshold", &m_BloomParams.Threshold, 0.01f, 0.0f, 1000.0f);
			ImGui::DragFloat("Knee", &m_BloomParams.Knee, 0.01f, 0.001f, 1.0f);
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
		m_UBCamera[frameIndex].WriteandFlushBuffer(&cameraUB);

		// Point Light
		UBPointLights pointLightUB{};
		m_Scene->UpdatePointLightUB(pointLightUB);
		m_UBPointLight[frameIndex].WriteandFlushBuffer(&pointLightUB);

		UBSpotLights spotLightUB{};
		m_Scene->UpdateSpotLightUB(spotLightUB);
		m_UBSpotLight[frameIndex].WriteandFlushBuffer(&spotLightUB);

		GeometryPass();
		BloomCompute();
		CompositePass();

		ResetDrawCommands();
	}

	void SceneRenderer::RenderLights()
	{ 
		Renderer::Submit([this]
		{
			VkCommandBuffer bindCmd = m_SceneCommandBuffer->RT_GetActiveCommandBuffer();
			m_LightPipeline->Bind(bindCmd);

			// Binding Point Light Descriptor Set
			m_PointLightShaderMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_LightPipeline);
		});

		// Point Lights
		for (auto pointLightPosition : m_PointLightPositions)
		{
			Renderer::Submit([this, pointLightPosition]
			{
				VK_CORE_PROFILE_FN("Render-PointLights");

				VkCommandBuffer drawCmd = m_SceneCommandBuffer->RT_GetActiveCommandBuffer();

				m_LightPipeline->SetPushConstants(drawCmd, (void*)&pointLightPosition, sizeof(glm::vec4));
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

				VkCommandBuffer drawCmd = m_SceneCommandBuffer->RT_GetActiveCommandBuffer();

				m_LightPipeline->SetPushConstants(drawCmd, (void*)&spotLightPosition, sizeof(glm::vec4));
				vkCmdDraw(drawCmd, 6, 1, 0, 0);
			});
		}
	}

	void SceneRenderer::SubmitMesh(std::shared_ptr<Mesh> mesh, std::shared_ptr<MaterialAsset> materialAsset, const glm::mat4& transform)
	{
		VK_CORE_PROFILE();

		if (!mesh || !materialAsset)
			return;

		auto meshSource = mesh->GetMeshSource();
		uint64_t meshHandle = mesh->Handle;
		uint64_t materialHandle = materialAsset->Handle;

		if (meshSource->GetVertexCount() == 0)
			return;

		for (uint32_t submeshIndex : mesh->GetSubmeshes())
		{
			MeshKey meshKey = { meshHandle, materialHandle, submeshIndex };
			auto& transformBuffer = m_MeshTransformMap[meshKey].Transforms.emplace_back();
			transformBuffer.MRow[0] = { transform[0][0], transform[1][0], transform[2][0], transform[3][0] };
			transformBuffer.MRow[1] = { transform[0][1], transform[1][1], transform[2][1], transform[3][1] };
			transformBuffer.MRow[2] = { transform[0][2], transform[1][2], transform[2][2], transform[3][2] };

			auto& dc = m_MeshDrawList[meshKey];
			dc.MeshInstance = mesh;
			dc.MaterialInstance = materialAsset->GetMaterial();
			dc.SubmeshIndex = submeshIndex;
			dc.TransformBuffer = m_MeshTransformMap[meshKey].TransformBuffer;
			dc.InstanceCount++;
		}
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

	void SceneRenderer::CompositePass()
	{
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Composite-Pass", DebugLabelColor::Red);

		Renderer::BeginRenderPass(m_SceneCommandBuffer, m_CompositePipeline->GetSpecification().RenderPass);

		Renderer::Submit([this] { m_CompositePipeline->SetPushConstants(m_SceneCommandBuffer->RT_GetActiveCommandBuffer(), &m_SceneSettings, sizeof(SceneSettings)); });
		Renderer::SubmitFullscreenQuad(m_SceneCommandBuffer, m_CompositePipeline, m_CompositeShaderMaterial);
		Renderer::EndRenderPass(m_SceneCommandBuffer, m_CompositePipeline->GetSpecification().RenderPass);

		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
	}

	void SceneRenderer::GeometryPass()
	{
		m_Scene->OnUpdateGeometry(this);
		m_Scene->OnUpdateLights(m_PointLightPositions, m_SpotLightPositions);

		Renderer::BeginRenderPass(m_SceneCommandBuffer, m_GeometryPipeline->GetSpecification().RenderPass);

		// Rendering Geometry
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Geometry-Pass", DebugLabelColor::Gold);

		Renderer::Submit([this]
		{
			VkCommandBuffer bindCmd = m_SceneCommandBuffer->RT_GetActiveCommandBuffer();
			m_GeometryPipeline->Bind(bindCmd);

			// Binding Static Geometry Descriptor Sets
			m_GeometryMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_GeometryPipeline);
		});

		for (auto& [mk, dc] : m_MeshDrawList)
			VulkanRenderer::RenderMesh(m_SceneCommandBuffer, dc.MeshInstance, dc.MaterialInstance, dc.SubmeshIndex, m_GeometryPipeline, dc.TransformBuffer, m_MeshTransformMap[mk].Transforms, dc.InstanceCount);

		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);

		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Skybox", DebugLabelColor::Orange);

		// Rendering Skybox
		Renderer::RenderSkybox(m_SceneCommandBuffer, m_SkyboxPipeline, m_SkyboxVBData, m_SkyboxMaterial, &m_SkyboxSettings);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);

		// Rendering Point Lights
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Lights", DebugLabelColor::Grey);

		RenderLights();

		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);

		Renderer::EndRenderPass(m_SceneCommandBuffer, m_GeometryPipeline->GetSpecification().RenderPass);

		// Copying Image for Bloom
		int frameIndex = Renderer::GetCurrentFrameIndex();

		VulkanRenderer::CopyVulkanImage(m_SceneCommandBuffer,
			m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetAttachment(true)[frameIndex],
			m_SceneRenderTextures[frameIndex]);

		VulkanRenderer::BlitVulkanImage(m_SceneCommandBuffer, m_SceneRenderTextures[frameIndex]);
	}

	void SceneRenderer::BloomCompute()
	{
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Bloom", DebugLabelColor::Blue);

		Renderer::Submit([this]
		{
			VK_CORE_PROFILE_FN("SceneRenderer::BloomCompute");

			VkCommandBuffer dispatchCmd = m_SceneCommandBuffer->RT_GetActiveCommandBuffer();
			m_BloomPipeline->Bind(dispatchCmd);

			// Prefilter
			m_LodAndMode.LOD = 0.0f;
			m_LodAndMode.Mode = 0.0f;

			m_BloomPrefilterShaderMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);

			const uint32_t mips = m_BloomTextures[0]->GetSpecification().MipLevels;
			glm::uvec2 bloomMipSize = m_BloomMipSize;

			m_BloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
			m_BloomPipeline->SetPushConstants(dispatchCmd, &m_BloomParams, sizeof(glm::vec2), sizeof(glm::vec2));
			m_BloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);

			for (uint32_t i = 1; i < mips; i++)
			{
				m_LodAndMode.LOD = float(i - 1);
				m_LodAndMode.Mode = 1.0f;

				int currentIdx = i - 1;

				m_BloomPingShaderMaterials[currentIdx]->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);
				bloomMipSize = m_BloomTextures[0]->GetMipSize(i);

				m_BloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
				m_BloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);

				m_LodAndMode.LOD = (float)i;
				
				m_BloomPongShaderMaterials[currentIdx]->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);

				m_BloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
				m_BloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);
			}

			// Upsample First
			// TODO: Could have to use VkImageSubresourceRange to set correct mip level
			m_LodAndMode.LOD = float(mips - 2);
			m_LodAndMode.Mode = 2.0f;

			m_BloomUpsampleFirstShaderMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);

			bloomMipSize = m_BloomTextures[2]->GetMipSize(mips - 1);

			m_BloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
			m_BloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);

			// Upsample Final
			for (int i = mips - 2; i >= 0; --i)
			{
				m_LodAndMode.LOD = (float)i;
				m_LodAndMode.Mode = 3.0f;

				m_BloomUpsampleShaderMaterials[i]->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);

				bloomMipSize = m_BloomTextures[2]->GetMipSize(i);

				m_BloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
				m_BloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);
			}
		});

		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
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

		m_PointLightPositions.clear();
		m_SpotLightPositions.clear();
	}

}