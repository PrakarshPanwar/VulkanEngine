#include "vulkanpch.h"
#include "SceneRenderer.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Core/Timer.h"
#include "VulkanCore/Renderer/Renderer.h"

#include "VulkanCore/Mesh/Mesh.h"
#include "Platform/Vulkan/VulkanAllocator.h"

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

			return std::make_shared<VulkanVertexBuffer>(skyboxVertices, sizeof(skyboxVertices));
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
		CreateDescriptorSets();
	}

	void SceneRenderer::CreatePipelines()
	{
		VertexBufferLayout vertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float3, "a_Tangent" },
			{ ShaderDataType::Float3, "a_Binormal" },
			{ ShaderDataType::Float3, "a_FragColor" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Int,    "a_TexIndex" }
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

			PipelineSpecification pointLightPipelineSpec;
			pointLightPipelineSpec.DebugName = "Point Light Pipeline";
			pointLightPipelineSpec.pShader = Renderer::GetShader("PointLight");
			pointLightPipelineSpec.Blend = true;
			pointLightPipelineSpec.RenderPass = geomPipelineSpec.RenderPass;

			m_GeometryPipeline = std::make_shared<VulkanPipeline>(geomPipelineSpec);
			m_PointLightPipeline = std::make_shared<VulkanPipeline>(pointLightPipelineSpec);
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
			m_BloomPipeline = std::make_shared<VulkanComputePipeline>(Renderer::GetShader("Bloom"));
		}
	}

	void SceneRenderer::CreateDescriptorSets()
	{
		auto device = VulkanContext::GetCurrentDevice();

		m_UBCamera.reserve(VulkanSwapChain::MaxFramesInFlight);
		m_UBPointLight.reserve(VulkanSwapChain::MaxFramesInFlight);
		m_UBSpotLight.reserve(VulkanSwapChain::MaxFramesInFlight);

		// Uniform Buffers
		for (int i = 0; i < VulkanSwapChain::MaxFramesInFlight; ++i)
		{
			m_UBCamera.emplace_back(sizeof(UBCamera));
			m_UBPointLight.emplace_back(sizeof(UBPointLights));
			m_UBSpotLight.emplace_back(sizeof(UBSpotLights));
		}

		m_BloomMipSize = (glm::uvec2(1920, 1080) + 1u) / 2u;
		m_BloomMipSize += 16u - m_BloomMipSize % 16u;

		ImageSpecification bloomRTSpec = {};
		bloomRTSpec.DebugName = "Bloom Compute Texture";
		bloomRTSpec.Width = m_BloomMipSize.x;
		bloomRTSpec.Height = m_BloomMipSize.y;
		bloomRTSpec.Format = ImageFormat::RGBA32F;
		bloomRTSpec.Usage = ImageUsage::Storage;
		bloomRTSpec.MipLevels = Utils::CalculateMipCount(m_BloomMipSize.x, m_BloomMipSize.y) - 2;
		
		m_BloomTextures.reserve(3);
		m_SceneRenderTextures.reserve(3);

		VkCommandBuffer barrierCmd = device->GetCommandBuffer();

		for (int i = 0; i < 3; i++)
		{
			VulkanImage& BloomTexture = m_BloomTextures.emplace_back(bloomRTSpec);
			BloomTexture.Invalidate();

			Utils::InsertImageMemoryBarrier(barrierCmd, BloomTexture.GetVulkanImageInfo().Image,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, BloomTexture.GetSpecification().MipLevels, 0, 1 });
		}

		ImageSpecification sceneRTSpec = {};
		sceneRTSpec.DebugName = "Scene Render Texture";
		sceneRTSpec.Width = 1920;
		sceneRTSpec.Height = 1080;
		sceneRTSpec.Format = ImageFormat::RGBA32F;
		sceneRTSpec.Usage = ImageUsage::Texture;
		sceneRTSpec.MipLevels = Utils::CalculateMipCount(1920, 1080);

		for (int i = 0; i < 3; i++)
		{
			VulkanImage& SceneTexture = m_SceneRenderTextures.emplace_back(sceneRTSpec);
			SceneTexture.Invalidate();

			Utils::InsertImageMemoryBarrier(barrierCmd, SceneTexture.GetVulkanImageInfo().Image,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, SceneTexture.GetSpecification().MipLevels, 0, 1 });
		}

		device->FlushCommandBuffer(barrierCmd);

		// Textures
		m_DiffuseMap = std::make_shared<VulkanTexture>("assets/meshes/CeramicVase2K/textures/antique_ceramic_vase_01_diff_2k.jpg");
		m_NormalMap = std::make_shared<VulkanTexture>("assets/meshes/CeramicVase2K/textures/antique_ceramic_vase_01_nor_gl_2k.png", ImageFormat::RGBA8_UNORM);
		m_ARMMap = std::make_shared<VulkanTexture>("assets/meshes/CeramicVase2K/textures/antique_ceramic_vase_01_arm_2k.png", ImageFormat::RGBA8_UNORM);

		m_DiffuseMap2 = std::make_shared<VulkanTexture>("assets/meshes/BrassVase2K/textures/brass_vase_03_diff_2k.jpg");
		m_NormalMap2 = std::make_shared<VulkanTexture>("assets/meshes/BrassVase2K/textures/brass_vase_03_nor_gl_2k.png", ImageFormat::RGBA8_UNORM);
		m_ARMMap2 = std::make_shared<VulkanTexture>("assets/meshes/BrassVase2K/textures/brass_vase_03_arm_2k.png", ImageFormat::RGBA8_UNORM);

#define USE_GOLD_MATERIAL 0
#if USE_GOLD_MATERIAL
		m_DiffuseMap3 = std::make_shared<VulkanTexture>("assets/textures/Gold/GoldDiffuse2.png");
		m_NormalMap3 = std::make_shared<VulkanTexture>("assets/textures/Gold/GoldNormalGL.png", ImageFormat::RGBA8_UNORM);
		m_ARMMap3 = std::make_shared<VulkanTexture>("assets/textures/Gold/GoldAORMNew.png", ImageFormat::RGBA8_UNORM);
#else
		m_DiffuseMap3 = std::make_shared<VulkanTexture>("assets/textures/StoneTiles/StoneTilesDiff.png");
		m_NormalMap3 = std::make_shared<VulkanTexture>("assets/textures/StoneTiles/StoneTilesNorGL.png", ImageFormat::RGBA8_UNORM);
		m_ARMMap3 = std::make_shared<VulkanTexture>("assets/textures/StoneTiles/StoneTilesARM.png", ImageFormat::RGBA8_UNORM);
#endif
		m_DiffuseMap4 = std::make_shared<VulkanTexture>("assets/textures/ConcreteWall/concrete_wall_006_diff_2k.png");
		m_NormalMap4 = std::make_shared<VulkanTexture>("assets/textures/ConcreteWall/concrete_wall_006_nor_gl_2k.png", ImageFormat::RGBA8_UNORM);
		m_ARMMap4 = std::make_shared<VulkanTexture>("assets/textures/ConcreteWall/concrete_wall_006_arm_2k.png", ImageFormat::RGBA8_UNORM);

		m_DiffuseMap5 = std::make_shared<VulkanTexture>("assets/textures/PlankFlooring/plank_flooring_diff_2k.png");
		m_NormalMap5 = std::make_shared<VulkanTexture>("assets/textures/PlankFlooring/plank_flooring_nor_gl_2k.png", ImageFormat::RGBA8_UNORM);
		m_ARMMap5 = std::make_shared<VulkanTexture>("assets/textures/PlankFlooring/plank_flooring_arm_2k.png", ImageFormat::RGBA8_UNORM);

		m_SRGBWhiteTexture = Renderer::GetWhiteTexture();
		m_UNORMWhiteTexture = Renderer::GetWhiteTexture(ImageFormat::RGBA8_UNORM);
		m_BloomDirtTexture = std::make_shared<VulkanTexture>("assets/textures/LensDirt.png");

		auto [filteredMap, irradianceMap] = VulkanRenderer::CreateEnviromentMap("assets/cubemaps/HDR/Birchwood4K.hdr");
		m_CubemapTexture = filteredMap;
		m_PrefilteredTexture = filteredMap;
		m_IrradianceTexture = irradianceMap;

		m_BRDFTexture = VulkanRenderer::CreateBRDFTexture();

		m_SkyboxVBData = Utils::CreateCubeModel();

		std::vector<VkDescriptorImageInfo> DiffuseMaps, ARMMaps, NormalMaps;
		DiffuseMaps.push_back(m_SRGBWhiteTexture->GetDescriptorImageInfo());
		DiffuseMaps.push_back(m_DiffuseMap->GetDescriptorImageInfo());
		DiffuseMaps.push_back(m_DiffuseMap2->GetDescriptorImageInfo());
		DiffuseMaps.push_back(m_DiffuseMap3->GetDescriptorImageInfo());
		DiffuseMaps.push_back(m_DiffuseMap4->GetDescriptorImageInfo());
		DiffuseMaps.push_back(m_DiffuseMap5->GetDescriptorImageInfo());
		NormalMaps.push_back(m_UNORMWhiteTexture->GetDescriptorImageInfo());
		NormalMaps.push_back(m_NormalMap->GetDescriptorImageInfo());
		NormalMaps.push_back(m_NormalMap2->GetDescriptorImageInfo());
		NormalMaps.push_back(m_NormalMap3->GetDescriptorImageInfo());
		NormalMaps.push_back(m_NormalMap4->GetDescriptorImageInfo());
		NormalMaps.push_back(m_NormalMap5->GetDescriptorImageInfo());
		ARMMaps.push_back(m_UNORMWhiteTexture->GetDescriptorImageInfo());
		ARMMaps.push_back(m_ARMMap->GetDescriptorImageInfo());
		ARMMaps.push_back(m_ARMMap2->GetDescriptorImageInfo());
		ARMMaps.push_back(m_ARMMap3->GetDescriptorImageInfo());
		ARMMaps.push_back(m_ARMMap4->GetDescriptorImageInfo());
		ARMMaps.push_back(m_ARMMap5->GetDescriptorImageInfo());

		// Writing in Descriptors
		auto vulkanDescriptorPool = Application::Get()->GetDescriptorPool();

		m_GeometryDescriptorSets.resize(VulkanSwapChain::MaxFramesInFlight);
		m_PointLightDescriptorSets.resize(VulkanSwapChain::MaxFramesInFlight);
		m_CompositeDescriptorSets.resize(VulkanSwapChain::MaxFramesInFlight);
		m_SkyboxDescriptorSets.resize(VulkanSwapChain::MaxFramesInFlight);

		// Geometry Descriptors
		std::vector<VulkanDescriptorWriter> geomDescriptorWriter(
			VulkanSwapChain::MaxFramesInFlight,
			{ *m_GeometryPipeline->GetDescriptorSetLayout(), *vulkanDescriptorPool });

		for (int i = 0; i < m_GeometryDescriptorSets.size(); ++i)
		{
			auto cameraUBInfo = m_UBCamera[i].GetDescriptorBufferInfo();
			geomDescriptorWriter[i].WriteBuffer(0, &cameraUBInfo);

			auto pointLightUBInfo = m_UBPointLight[i].GetDescriptorBufferInfo();
			geomDescriptorWriter[i].WriteBuffer(1, &pointLightUBInfo);

			auto spotLightUBInfo = m_UBSpotLight[i].GetDescriptorBufferInfo();
			geomDescriptorWriter[i].WriteBuffer(2, &spotLightUBInfo);

			geomDescriptorWriter[i].WriteImage(3, DiffuseMaps);
			geomDescriptorWriter[i].WriteImage(4, NormalMaps);
			geomDescriptorWriter[i].WriteImage(5, ARMMaps);
			
			// Irradiance Map
			VkDescriptorImageInfo irradianceMapInfo = m_IrradianceTexture->GetDescriptorImageInfo();
			geomDescriptorWriter[i].WriteImage(6, &irradianceMapInfo);

			// BRDF LUT Texture
			VkDescriptorImageInfo brdfTextureInfo = m_BRDFTexture->GetDescriptorInfo();
			geomDescriptorWriter[i].WriteImage(7, &brdfTextureInfo);

			// Prefiltered Map
			VkDescriptorImageInfo prefilteredMapInfo = m_PrefilteredTexture->GetDescriptorImageInfo();
			geomDescriptorWriter[i].WriteImage(8, &prefilteredMapInfo);

			geomDescriptorWriter[i].Build(m_GeometryDescriptorSets[i]);
		}

		// Point Light Descriptors
		std::vector<VulkanDescriptorWriter> pointLightDescriptorWriter(
			VulkanSwapChain::MaxFramesInFlight,
			{ *m_PointLightPipeline->GetDescriptorSetLayout(), *vulkanDescriptorPool });

		for (int i = 0; i < m_PointLightDescriptorSets.size(); ++i)
		{
			auto cameraUBInfo = m_UBCamera[i].GetDescriptorBufferInfo();
			pointLightDescriptorWriter[i].WriteBuffer(0, &cameraUBInfo);

			bool success = pointLightDescriptorWriter[i].Build(m_PointLightDescriptorSets[i]);
			VK_CORE_ASSERT(success, "Failed to Write to Descriptor Set!");
		}

		m_BloomPrefilterSets.resize(VulkanSwapChain::MaxFramesInFlight);
		m_BloomPingSets.resize(VulkanSwapChain::MaxFramesInFlight);
		m_BloomPongSets.resize(VulkanSwapChain::MaxFramesInFlight);
		m_BloomUpsampleFirstSets.resize(VulkanSwapChain::MaxFramesInFlight);
		m_BloomUpsampleSets.resize(VulkanSwapChain::MaxFramesInFlight);

		// Bloom Compute Descriptors
		std::vector<VulkanDescriptorWriter> bloomDescriptorWriter(
			VulkanSwapChain::MaxFramesInFlight,
			{ *m_BloomPipeline->GetDescriptorSetLayout(), *vulkanDescriptorPool });

		const uint32_t mipCount = m_BloomTextures[0].GetSpecification().MipLevels;
		for (int i = 0; i < m_BloomPrefilterSets.size(); i++)
		{
			// Set A : Prefiltered Sets
			// Binding 0(o_Image): BloomTex[0]
			// Binding 1(u_Texture): RenderTex
			// Binding 2(u_BloomTexture): RenderTex

			VkDescriptorImageInfo outputImageInfo = m_BloomTextures[0].GetDescriptorInfo();
			bloomDescriptorWriter[i].WriteImage(0, &outputImageInfo);

			VkDescriptorImageInfo texInfo = m_SceneRenderTextures[i].GetDescriptorInfo();
			bloomDescriptorWriter[i].WriteImage(1, &texInfo);
			bloomDescriptorWriter[i].WriteImage(2, &texInfo);

			bool success = bloomDescriptorWriter[i].Build(m_BloomPrefilterSets[i]);
			VK_CORE_ASSERT(success, "Failed to Write to Descriptor Set!");

			m_BloomPingSets[i].resize(mipCount - 1);
			m_BloomPongSets[i].resize(mipCount - 1);
			for (uint32_t j = 1; j < mipCount; ++j)
			{
				// Set B: Downsampling(Ping)
				// Binding 0(o_Image): BloomTex[1]
				// Binding 1(u_Texture): BloomTex[0]
				// Binding 2(u_BloomTexture): RenderTex
				int currentIdx = j - 1;

				outputImageInfo = m_BloomTextures[1].GetDescriptorInfo();
				outputImageInfo.imageView = m_BloomTextures[1].CreateImageViewSingleMip(j);
				bloomDescriptorWriter[i].WriteImage(0, &outputImageInfo);

				texInfo = m_BloomTextures[0].GetDescriptorInfo();
				bloomDescriptorWriter[i].WriteImage(1, &texInfo);

				success = bloomDescriptorWriter[i].Build(m_BloomPingSets[i][currentIdx]);
				VK_CORE_ASSERT(success, "Failed to Write to Descriptor Set!");

				// Set C: Downsampling(Pong)
				// Binding 0(o_Image): BloomTex[0]
				// Binding 1(u_Texture): BloomTex[1]
				// Binding 2(u_BloomTexture): RenderTex

				outputImageInfo = m_BloomTextures[0].GetDescriptorInfo();
				outputImageInfo.imageView = m_BloomTextures[0].CreateImageViewSingleMip(j);
				bloomDescriptorWriter[i].WriteImage(0, &outputImageInfo);

				texInfo = m_BloomTextures[1].GetDescriptorInfo();
				bloomDescriptorWriter[i].WriteImage(1, &texInfo);

				success = bloomDescriptorWriter[i].Build(m_BloomPongSets[i][currentIdx]);
				VK_CORE_ASSERT(success, "Failed to Write to Descriptor Set!");
			}

			// Set D: Upsample First
			// Binding 0(o_Image): BloomTex[2]
			// Binding 1(u_Texture): BloomTex[0]
			// Binding 2(u_BloomTexture): RenderTex

			outputImageInfo = m_BloomTextures[2].GetDescriptorInfo();
			outputImageInfo.imageView = m_BloomTextures[2].CreateImageViewSingleMip(mipCount - 1);
			bloomDescriptorWriter[i].WriteImage(0, &outputImageInfo);

			texInfo = m_BloomTextures[0].GetDescriptorInfo();
			bloomDescriptorWriter[i].WriteImage(1, &texInfo);

			success = bloomDescriptorWriter[i].Build(m_BloomUpsampleFirstSets[i]);
			VK_CORE_ASSERT(success, "Failed to Write to Descriptor Set!");

			// Set E: Upsample
			// Binding 0(o_Image): BloomTex[2]
			// Binding 1(u_Texture): BloomTex[0]
			// Binding 2(u_BloomTexture): BloomTex[2]

			m_BloomUpsampleSets[i].resize(mipCount - 1);
			for (int j = mipCount - 2; j >= 0; --j)
			{
				outputImageInfo.imageView = m_BloomTextures[2].CreateImageViewSingleMip(j);
				bloomDescriptorWriter[i].WriteImage(0, &outputImageInfo);

				VkDescriptorImageInfo bloomTexInfo = m_BloomTextures[2].GetDescriptorInfo();
				bloomDescriptorWriter[i].WriteImage(2, &bloomTexInfo);

				success = bloomDescriptorWriter[i].Build(m_BloomUpsampleSets[i][j]);
				VK_CORE_ASSERT(success, "Failed to Write to Descriptor Set!");
			}
		}

		// Composite Descriptors
		std::vector<VulkanDescriptorWriter> compDescriptorWriter(
			VulkanSwapChain::MaxFramesInFlight,
			{ *m_CompositePipeline->GetDescriptorSetLayout(), *vulkanDescriptorPool });

		for (int i = 0; i < m_CompositeDescriptorSets.size(); ++i)
		{
			VkDescriptorImageInfo imagesInfo = m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetResolveAttachment()[i].GetDescriptorInfo();
			VkDescriptorImageInfo bloomTexInfo = m_BloomTextures[2].GetDescriptorInfo();
			VkDescriptorImageInfo bloomDirtTexInfo = m_BloomDirtTexture->GetDescriptorImageInfo();

			compDescriptorWriter[i].WriteImage(0, &imagesInfo);
			compDescriptorWriter[i].WriteImage(1, &bloomTexInfo);
			compDescriptorWriter[i].WriteImage(2, &bloomDirtTexInfo);

			bool success = compDescriptorWriter[i].Build(m_CompositeDescriptorSets[i]);
			VK_CORE_ASSERT(success, "Failed to Write to Descriptor Set!");
		}

		// Skybox Descriptors
		std::vector<VulkanDescriptorWriter> skyboxDescriptorWriter(
			VulkanSwapChain::MaxFramesInFlight,
			{ *m_SkyboxPipeline->GetDescriptorSetLayout(), *vulkanDescriptorPool });

		for (int i = 0; i < m_SkyboxDescriptorSets.size(); ++i)
		{
			auto cameraUBInfo = m_UBCamera[i].GetDescriptorBufferInfo();
			skyboxDescriptorWriter[i].WriteBuffer(0, &cameraUBInfo);

			VkDescriptorImageInfo imageInfo = m_CubemapTexture->GetDescriptorImageInfo();
			skyboxDescriptorWriter[i].WriteImage(1, &imageInfo);

			bool success = skyboxDescriptorWriter[i].Build(m_SkyboxDescriptorSets[i]);
			VK_CORE_ASSERT(success, "Failed to Write to Descriptor Set!");
		}
		
		m_BloomDebugImage = ImGuiLayer::AddTexture(m_BloomTextures[2]);
	}

	void SceneRenderer::Release()
	{
		// Deleting all Transforms
		Mesh::ClearAllMeshes();
	}

	void SceneRenderer::RecreateScene()
{
		VK_CORE_INFO("Scene has been Recreated!");
		m_SceneRenderPass->RecreateFramebuffers(m_ViewportSize.x, m_ViewportSize.y);
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

	void SceneRenderer::RenderScene(EditorCamera& camera)
	{
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

	void SceneRenderer::SubmitMesh(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, const glm::mat4& transform)
	{
		auto meshSource = mesh->GetMeshSource();
		uint64_t meshHandle = meshSource->GetMeshHandle();

		if (meshSource->GetVertexCount() == 0)
			return;

		for (uint32_t submeshIndex : mesh->GetSubmeshes())
		{
			MeshKey meshKey = { meshHandle, submeshIndex };
			auto& transformBuffer = m_MeshTransformMap[meshKey].emplace_back();
			transformBuffer.MRow[0] = { transform[0][0], transform[1][0], transform[2][0], transform[3][0] };
			transformBuffer.MRow[1] = { transform[0][1], transform[1][1], transform[2][1], transform[3][1] };
			transformBuffer.MRow[2] = { transform[0][2], transform[1][2], transform[2][2], transform[3][2] };

			auto& dc = m_MeshDrawList[meshKey];
			dc.MeshInstance = mesh;
			dc.MaterialInstance = material;
			dc.SubmeshIndex = submeshIndex;
			dc.TransformBuffer = mesh->GetTransformBuffer(meshHandle);
			dc.InstanceCount++;
		}
	}

	void SceneRenderer::CompositePass()
	{
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		Renderer::BeginRenderPass(m_SceneCommandBuffer, m_CompositePipeline->GetSpecification().RenderPass);
		m_CompositePipeline->SetPushConstants(m_SceneCommandBuffer->GetActiveCommandBuffer(), &m_SceneSettings, sizeof(SceneSettings));
		Renderer::SubmitFullscreenQuad(m_SceneCommandBuffer, m_CompositePipeline, m_CompositeDescriptorSets);

		Renderer::EndRenderPass(m_SceneCommandBuffer, m_CompositePipeline->GetSpecification().RenderPass);

		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
	}

	void SceneRenderer::GeometryPass()
	{
		m_Scene->OnUpdateGeometry(this);

		Renderer::BeginRenderPass(m_SceneCommandBuffer, m_GeometryPipeline->GetSpecification().RenderPass);

		// Rendering Geometry
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		auto drawCmd = m_SceneCommandBuffer->GetActiveCommandBuffer();
		auto dstSet = m_GeometryDescriptorSets[Renderer::GetCurrentFrameIndex()];

		m_GeometryPipeline->Bind(drawCmd);

		vkCmdBindDescriptorSets(drawCmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_GeometryPipeline->GetVulkanPipelineLayout(),
			0, 1, &dstSet,
			0, nullptr);

		for (auto& [mk, dc] : m_MeshDrawList)
			VulkanRenderer::RenderMesh(m_SceneCommandBuffer, dc.MeshInstance, dc.MaterialInstance, dc.SubmeshIndex, m_GeometryPipeline, dc.TransformBuffer, m_MeshTransformMap[mk], dc.InstanceCount);

		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);

		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		// Rendering Skybox
		Renderer::RenderSkybox(m_SceneCommandBuffer, m_SkyboxPipeline, m_SkyboxVBData, m_SkyboxDescriptorSets, &m_SkyboxSettings);
		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);

		// Rendering Point Lights
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);
		m_Scene->OnUpdateLights(m_SceneCommandBuffer, m_PointLightPipeline, m_PointLightDescriptorSets);
		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);

		Renderer::EndRenderPass(m_SceneCommandBuffer, m_GeometryPipeline->GetSpecification().RenderPass);

		// Copying Image for Bloom
		int frameIndex = Renderer::GetCurrentFrameIndex();

		VulkanRenderer::CopyVulkanImage(m_SceneCommandBuffer->GetActiveCommandBuffer(),
			m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetResolveAttachment()[frameIndex],
			m_SceneRenderTextures[frameIndex]);

		VulkanRenderer::BlitVulkanImage(m_SceneCommandBuffer->GetActiveCommandBuffer(), m_SceneRenderTextures[frameIndex]);
	}

	void SceneRenderer::BloomCompute()
	{
		int frameIndex = Renderer::GetCurrentFrameIndex();

		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		VkCommandBuffer dispatchCmd = m_SceneCommandBuffer->GetActiveCommandBuffer();
		m_BloomPipeline->Bind(dispatchCmd);

		// Prefilter
		m_LodAndMode.LOD = 0.0f;
		m_LodAndMode.Mode = 0.0f;

		vkCmdBindDescriptorSets(dispatchCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			m_BloomPipeline->GetVulkanPipelineLayout(), 0, 1,
			&m_BloomPrefilterSets[frameIndex], 0, nullptr);

		const uint32_t mips = m_BloomTextures[0].GetSpecification().MipLevels;
		glm::uvec2 bloomMipSize = m_BloomMipSize;

		m_BloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
		m_BloomPipeline->SetPushConstants(dispatchCmd, &m_BloomParams, sizeof(glm::vec2), sizeof(glm::vec2));
		m_BloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);

		for (uint32_t i = 1; i < mips; i++)
		{
			m_LodAndMode.LOD = float(i - 1);
			m_LodAndMode.Mode = 1.0f;

			int currentIdx = i - 1;

			// Downsample(Ping)
			vkCmdBindDescriptorSets(dispatchCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
				m_BloomPipeline->GetVulkanPipelineLayout(), 0, 1,
				&m_BloomPingSets[frameIndex][currentIdx], 0, nullptr);

			bloomMipSize = m_BloomTextures[0].GetMipSize(i);

			m_BloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
			m_BloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);

			m_LodAndMode.LOD = (float)i;

			// Downsample(Pong)
			vkCmdBindDescriptorSets(dispatchCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
				m_BloomPipeline->GetVulkanPipelineLayout(), 0, 1,
				&m_BloomPongSets[frameIndex][currentIdx], 0, nullptr);

			m_BloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
			m_BloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);
		}

		// Upsample First
		// TODO: Could have to use VkImageSubresourceRange to set correct mip level
		m_LodAndMode.LOD = float(mips - 2);
		m_LodAndMode.Mode = 2.0f;

		vkCmdBindDescriptorSets(dispatchCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			m_BloomPipeline->GetVulkanPipelineLayout(), 0, 1,
			&m_BloomUpsampleFirstSets[frameIndex], 0, nullptr);

		bloomMipSize = m_BloomTextures[2].GetMipSize(mips - 1);

		m_BloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
		m_BloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);

		// Upsample Final
		for (int i = mips - 2; i >= 0; --i)
		{
			m_LodAndMode.LOD = (float)i;
			m_LodAndMode.Mode = 3.0f;

			vkCmdBindDescriptorSets(dispatchCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
				m_BloomPipeline->GetVulkanPipelineLayout(), 0, 1,
				&m_BloomUpsampleSets[frameIndex][i], 0, nullptr);

			bloomMipSize = m_BloomTextures[2].GetMipSize(i);

			m_BloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
			m_BloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);
		}

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
			m_MeshTransformMap[mk].clear();
			dc.InstanceCount = 0;
		}
	}

}