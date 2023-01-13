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

		static std::shared_ptr<Mesh> CreateCubeModel()
		{
			MeshBuilder modelBuilder{};

			modelBuilder.Vertices = {

				{ { -1.0f,  1.0f, -1.0f } },
				{ { -1.0f, -1.0f, -1.0f } },
				{ {  1.0f, -1.0f, -1.0f } },
				{ {  1.0f, -1.0f, -1.0f } },
				{ {  1.0f,  1.0f, -1.0f } },
				{ { -1.0f,  1.0f, -1.0f } },

				{ { -1.0f, -1.0f,  1.0f } },
				{ { -1.0f, -1.0f, -1.0f } },
				{ { -1.0f,  1.0f, -1.0f } },
				{ { -1.0f,  1.0f, -1.0f } },
				{ { -1.0f,  1.0f,  1.0f } },
				{ { -1.0f, -1.0f,  1.0f } },

				{ {  1.0f, -1.0f, -1.0f } },
				{ {  1.0f, -1.0f,  1.0f } },
				{ {  1.0f,  1.0f,  1.0f } },
				{ {  1.0f,  1.0f,  1.0f } },
				{ {  1.0f,  1.0f, -1.0f } },
				{ {  1.0f, -1.0f, -1.0f } },

				{ { -1.0f, -1.0f,  1.0f } },
				{ { -1.0f,  1.0f,  1.0f } },
				{ {  1.0f,  1.0f,  1.0f } },
				{ {  1.0f,  1.0f,  1.0f } },
				{ {  1.0f, -1.0f,  1.0f } },
				{ { -1.0f, -1.0f,  1.0f } },

				{ { -1.0f,  1.0f, -1.0f } },
				{ {  1.0f,  1.0f, -1.0f } },
				{ {  1.0f,  1.0f,  1.0f } },
				{ {  1.0f,  1.0f,  1.0f } },
				{ { -1.0f,  1.0f,  1.0f } },
				{ { -1.0f,  1.0f, -1.0f } },

				{ { -1.0f, -1.0f, -1.0f } },
				{ { -1.0f, -1.0f,  1.0f } },
				{ {  1.0f, -1.0f, -1.0f } },
				{ {  1.0f, -1.0f, -1.0f } },
				{ { -1.0f, -1.0f,  1.0f } },
				{ {  1.0f, -1.0f,  1.0f } }
			};

			return std::make_shared<Mesh>(modelBuilder);
		}

		static std::shared_ptr<Mesh> CreateCubeModel(glm::vec3 color)
		{
			MeshBuilder modelBuilder{};

			modelBuilder.Vertices = {

				// Left Face
				{ { -1.0f, -1.0f, -1.0f }, color },
				{ { -1.0f,  1.0f,  1.0f }, color },
				{ { -1.0f, -1.0f,  1.0f }, color },
				{ { -1.0f,  1.0f, -1.0f }, color },

				// Right Face
				{ {  1.0f, -1.0f, -1.0f }, color },
				{ {  1.0f,  1.0f,  1.0f }, color },
				{ {  1.0f, -1.0f,  1.0f }, color },
				{ {  1.0f,  1.0f, -1.0f }, color },

				// Top Face 
				{ { -1.0f, -1.0f, -1.0f }, color },
				{ {  1.0f, -1.0f,  1.0f }, color },
				{ { -1.0f, -1.0f,  1.0f }, color },
				{ {  1.0f, -1.0f, -1.0f }, color },

				// Bottom Face
				{ { -1.0f,  1.0f, -1.0f }, color },
				{ {  1.0f,  1.0f,  1.0f }, color },
				{ { -1.0f,  1.0f,  1.0f }, color },
				{ {  1.0f,  1.0f, -1.0f }, color },

				// Front Face
				{ { -1.0f, -1.0f,  1.0f }, color },
				{ {  1.0f,  1.0f,  1.0f }, color },
				{ { -1.0f,  1.0f,  1.0f }, color },
				{ {  1.0f, -1.0f,  1.0f }, color },

				// Back Face
				{ { -1.0f, -1.0f, -1.0f }, color },
				{ {  1.0f,  1.0f, -1.0f }, color },
				{ { -1.0f,  1.0f, -1.0f }, color },
				{ {  1.0f, -1.0f, -1.0f }, color }
			};

			modelBuilder.Indices = { 0,  1,  2,  0,  3,  1,  4,  5,  6,  4,  7,  5,  8,  9,  10, 8,  11, 9,
							  12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21 };

			return std::make_shared<Mesh>(modelBuilder);
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
		// Geometry and Point Light Pipeline
		{
			FramebufferSpecification geomFramebufferSpec;
			geomFramebufferSpec.Width = 1920;
			geomFramebufferSpec.Height = 1080;
			geomFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::DEPTH24STENCIL8 };
			geomFramebufferSpec.Samples = 8;

			RenderPassSpecification geomRenderPassSpec;
			geomRenderPassSpec.TargetFramebuffer = std::make_shared<VulkanFramebuffer>(geomFramebufferSpec);

			PipelineSpecification geomPipelineSpec;
			geomPipelineSpec.pShader = Renderer::GetShader("CoreShader");
			geomPipelineSpec.RenderPass = std::make_shared<VulkanRenderPass>(geomRenderPassSpec);
			geomPipelineSpec.Layout = { Vertex::GetBindingDescriptions(), Vertex::GetAttributeDescriptions() };

			PipelineSpecification pointLightPipelineSpec;
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
			compPipelineSpec.pShader = Renderer::GetShader("SceneComposite");
			compPipelineSpec.RenderPass = std::make_shared<VulkanRenderPass>(compRenderPassSpec);
			compPipelineSpec.DepthTest = false;
			compPipelineSpec.DepthWrite = false;

			m_CompositePipeline = std::make_shared<VulkanPipeline>(compPipelineSpec);
		}

		// Skybox Pipeline
		{
			PipelineSpecification skyboxPipelineSpec;
			skyboxPipelineSpec.pShader = Renderer::GetShader("Skybox");
			skyboxPipelineSpec.Layout = { Vertex::GetBindingDescriptions(), Vertex::GetAttributeDescriptions() };
			skyboxPipelineSpec.RenderPass = m_GeometryPipeline->GetSpecification().RenderPass;

			m_SkyboxPipeline = std::make_shared<VulkanPipeline>(skyboxPipelineSpec);
		}

#if BLOOM_COMPUTE_SHADER
		// Compute Testing Pipeline
		{
			m_BloomPipeline = std::make_shared<VulkanComputePipeline>(Renderer::GetShader("Bloom"));
		}
#endif
	}

	void SceneRenderer::CreateDescriptorSets()
	{
		auto device = VulkanContext::GetCurrentDevice();

		m_UBCamera.reserve(VulkanSwapChain::MaxFramesInFlight);
		m_UBPointLight.reserve(VulkanSwapChain::MaxFramesInFlight);
		m_UBSceneData.reserve(VulkanSwapChain::MaxFramesInFlight);
		m_UBBloomParams.reserve(VulkanSwapChain::MaxFramesInFlight);
		m_UBBloomLod.reserve(VulkanSwapChain::MaxFramesInFlight);

		// Uniform Buffers
		for (int i = 0; i < VulkanSwapChain::MaxFramesInFlight; ++i)
		{
			m_UBCamera.emplace_back(sizeof(Camera));
			m_UBPointLight.emplace_back(sizeof(UBPointLights));
			m_UBSceneData.emplace_back(sizeof(SceneSettings));
#if BLOOM_COMPUTE_SHADER
			m_UBBloomLod.emplace_back(sizeof(LodAndMode));
			m_UBBloomParams.emplace_back(sizeof(BloomParams));
#endif
		}

#if BLOOM_COMPUTE_SHADER
		m_BloomMipSize = { 1920, 1080 };
		m_BloomMipSize /= 2;
		m_BloomMipSize += glm::uvec2{ 16 - m_BloomMipSize.x % 16, 16 - m_BloomMipSize.y % 16 };

		ImageSpecification imageSpec = {};
		imageSpec.Width = m_BloomMipSize.x;
		imageSpec.Height = m_BloomMipSize.y;
		imageSpec.Format = ImageFormat::RGBA16F;
		imageSpec.Usage = ImageUsage::Storage;
		imageSpec.MipLevels = Utils::CalculateMipCount(1920, 1080) - 4;
		
		m_BloomTextures.reserve(3);
		m_SceneCopyImages.reserve(3);

#define USE_MEMORY_BARRIER 1
	#if USE_MEMORY_BARRIER
		VkCommandBuffer barrierCmd = device->GetCommandBuffer();
	#endif

		for (int i = 0; i < 3; i++)
		{
			VulkanImage& BloomTexture = m_BloomTextures.emplace_back(imageSpec);
			BloomTexture.Invalidate();

	#if USE_MEMORY_BARRIER
			Utils::InsertImageMemoryBarrier(barrierCmd, BloomTexture.GetVulkanImageInfo().Image,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, BloomTexture.GetSpecification().MipLevels, 0, 1 });
	#endif
		}

	#if USE_MEMORY_BARRIER
		device->FlushCommandBuffer(barrierCmd);
	#endif
#endif

		// Textures
		m_DiffuseMap = std::make_shared<VulkanTexture>("assets/models/CeramicVase2K/textures/antique_ceramic_vase_01_diff_2k.jpg");
		m_NormalMap = std::make_shared<VulkanTexture>("assets/models/CeramicVase2K/textures/antique_ceramic_vase_01_nor_gl_2k.jpg");
		m_SpecularMap = std::make_shared<VulkanTexture>("assets/textures/PlainSnow/SnowSpecular.jpg");

		m_DiffuseMap2 = std::make_shared<VulkanTexture>("assets/textures/DeformedSnow/SnowDiffuse.jpg");
		m_NormalMap2 = std::make_shared<VulkanTexture>("assets/textures/DeformedSnow/SnowNormalGL.png");
		m_SpecularMap2 = std::make_shared<VulkanTexture>("assets/textures/DeformedSnow/SnowSpecular.jpg");

		m_DiffuseMap3 = std::make_shared<VulkanTexture>("assets/textures/Marble/MarbleDiff.png");
		m_NormalMap3 = std::make_shared<VulkanTexture>("assets/textures/Marble/MarbleNormalGL.png");
		m_SpecularMap3 = std::make_shared<VulkanTexture>("assets/textures/Marble/MarbleSpec.jpg");

		auto [filteredMap, irradianceMap] = VulkanRenderer::CreateEnviromentMap("assets/cubemaps/HDR/LagoMountains4K.hdr");
		m_CubemapTexture = filteredMap;
		m_SkyboxMesh = Utils::CreateCubeModel();

		std::vector<VkDescriptorImageInfo> DiffuseMaps, SpecularMaps, NormalMaps;
		DiffuseMaps.push_back(m_DiffuseMap->GetDescriptorImageInfo());
		DiffuseMaps.push_back(m_DiffuseMap2->GetDescriptorImageInfo());
		DiffuseMaps.push_back(m_DiffuseMap3->GetDescriptorImageInfo());
		NormalMaps.push_back(m_NormalMap->GetDescriptorImageInfo());
		NormalMaps.push_back(m_NormalMap2->GetDescriptorImageInfo());
		NormalMaps.push_back(m_NormalMap3->GetDescriptorImageInfo());
		SpecularMaps.push_back(m_SpecularMap->GetDescriptorImageInfo());
		SpecularMaps.push_back(m_SpecularMap2->GetDescriptorImageInfo());
		SpecularMaps.push_back(m_SpecularMap3->GetDescriptorImageInfo());

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

			geomDescriptorWriter[i].WriteImage(2, DiffuseMaps);
			geomDescriptorWriter[i].WriteImage(3, NormalMaps);
			geomDescriptorWriter[i].WriteImage(4, SpecularMaps);

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

#if BLOOM_COMPUTE_SHADER
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
			//outputImageInfo.imageView = m_BloomTextures[0].CreateImageViewSingleMip(0);
			bloomDescriptorWriter[i].WriteImage(0, &outputImageInfo);

			VkDescriptorImageInfo texInfo = m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetResolveAttachment()[i].GetDescriptorInfo();
			bloomDescriptorWriter[i].WriteImage(1, &texInfo);
			bloomDescriptorWriter[i].WriteImage(2, &texInfo);

			auto lodUBInfo = m_UBBloomLod[i].GetDescriptorBufferInfo();
			bloomDescriptorWriter[i].WriteBuffer(3, &lodUBInfo);

			auto bloomParamUBInfo = m_UBBloomParams[i].GetDescriptorBufferInfo();
			bloomDescriptorWriter[i].WriteBuffer(4, &bloomParamUBInfo);

			bool success = bloomDescriptorWriter[i].Build(m_BloomPrefilterSets[i]);
			VK_CORE_ASSERT(success, "Failed to Write to Descriptor Set!");

			m_BloomPingSets[i].resize(mipCount - 1);
			m_BloomPongSets[i].resize(mipCount - 1);
			for (int j = 1; j < mipCount; ++j)
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
#endif

		// Composite Descriptors
		std::vector<VulkanDescriptorWriter> compDescriptorWriter(
			VulkanSwapChain::MaxFramesInFlight,
			{ *m_CompositePipeline->GetDescriptorSetLayout(), *vulkanDescriptorPool });

		for (int i = 0; i < m_CompositeDescriptorSets.size(); ++i)
		{
			auto sceneUBInfo = m_UBSceneData[i].GetDescriptorBufferInfo();
			compDescriptorWriter[i].WriteBuffer(1, &sceneUBInfo);

			VkDescriptorImageInfo imagesInfo = m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetResolveAttachment()[i].GetDescriptorInfo();
			VkDescriptorImageInfo bloomTexInfo = m_BloomTextures[2].GetDescriptorInfo();
			
			compDescriptorWriter[i].WriteImage(0, &imagesInfo);
			compDescriptorWriter[i].WriteImage(2, &bloomTexInfo);

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
		auto device = VulkanContext::GetCurrentDevice();

		vkFreeCommandBuffers(device->GetVulkanDevice(), device->GetCommandPool(), 
			static_cast<uint32_t>(m_SceneCommandBuffers.size()), m_SceneCommandBuffers.data());
	}

	void SceneRenderer::RecreateScene()
{
		VK_CORE_INFO("Scene has been Recreated!");
		m_SceneRenderPass->RecreateFramebuffers(m_ViewportSize.x, m_ViewportSize.y);
	}

	void SceneRenderer::OnImGuiRender()
	{
		ImGui::Begin("Scene Renderer");
		ImGui::DragFloat("Exposure Intensity", &m_SceneSettings.Exposure, 0.01f, 0.0f, 20.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::DragFloat("Skybox LOD", &m_SkyboxLOD, 0.01f, 0.0f, 11.0f);

		if (ImGui::TreeNode("Scene Renderer Stats##GPUPerf"))
		{
			Renderer::RetrieveQueryPoolResults();

			ImGui::Text("Geometry Pass: %lluns", Renderer::GetQueryTime(0));
			ImGui::Text("Skybox Pass: %lluns", Renderer::GetQueryTime(2));
			ImGui::Text("Composite Pass: %lluns", Renderer::GetQueryTime(3));
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("BloomSettings"))
		{
			ImGui::DragFloat("Threshold", &m_BloomParams.Threshold, 0.01f);
			ImGui::DragFloat("Knee", &m_BloomParams.Knee, 0.01f);
			ImGui::TreePop();
		}

		ImGui::Image(m_BloomDebugImage, ImGui::GetContentRegionAvail());
		ImGui::End(); // End of Scene Renderer Window
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

		// Scene Data
		m_UBSceneData[frameIndex].WriteandFlushBuffer(&m_SceneSettings);
#if BLOOM_COMPUTE_SHADER
		m_UBBloomParams[frameIndex].WriteandFlushBuffer(&m_BloomParams);
#endif

		GeometryPass();
#if BLOOM_COMPUTE_SHADER
		BloomCompute();
#endif
		CompositePass();
	}

	void SceneRenderer::CompositePass()
	{
		Renderer::BeginGPUPerfMarker();

		Renderer::BeginRenderPass(m_CompositePipeline->GetSpecification().RenderPass);
		Renderer::SubmitFullscreenQuad(m_CompositePipeline, m_CompositeDescriptorSets);
		Renderer::EndRenderPass(m_CompositePipeline->GetSpecification().RenderPass);

		Renderer::EndGPUPerfMarker();
	}

	void SceneRenderer::GeometryPass()
	{
		Renderer::BeginRenderPass(m_GeometryPipeline->GetSpecification().RenderPass);

		// Rendering Geometry
		Renderer::BeginGPUPerfMarker();
		m_Scene->OnUpdateGeometry(m_SceneCommandBuffers, m_GeometryPipeline, m_GeometryDescriptorSets);
		Renderer::EndGPUPerfMarker();

		// Rendering Point Lights
		Renderer::BeginGPUPerfMarker();
		m_Scene->OnUpdateLights(m_SceneCommandBuffers, m_PointLightPipeline, m_PointLightDescriptorSets);
		Renderer::EndGPUPerfMarker();

		Renderer::BeginGPUPerfMarker();

		// Rendering Skybox
		Renderer::RenderSkybox(m_SkyboxPipeline, m_SkyboxMesh, m_SkyboxDescriptorSets, &m_SkyboxLOD);
		Renderer::EndGPUPerfMarker();

		Renderer::EndRenderPass(m_GeometryPipeline->GetSpecification().RenderPass);
	}

	void SceneRenderer::BloomCompute()
	{
		int frameIndex = Renderer::GetCurrentFrameIndex();

		VkCommandBuffer dispatchCmd = m_SceneCommandBuffers[frameIndex];
		m_BloomPipeline->Bind(dispatchCmd);

		// Prefilter
		m_LodAndMode.LOD = 0.0f;
		m_LodAndMode.Mode = 0.0f;
		m_UBBloomLod[frameIndex].WriteandFlushBuffer(&m_LodAndMode);

		vkCmdBindDescriptorSets(dispatchCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			m_BloomPipeline->GetVulkanPipelineLayout(), 0, 1,
			&m_BloomPrefilterSets[frameIndex], 0, nullptr);

		const uint32_t mips = m_BloomTextures[0].GetSpecification().MipLevels;
		glm::uvec2 bloomMipSize = m_BloomMipSize;

		m_BloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);

		for (int i = 1; i < mips; i++)
		{
			m_LodAndMode.LOD = float(i - 1);
			m_LodAndMode.Mode = 1.0f;
			m_UBBloomLod[frameIndex].WriteandFlushBuffer(&m_LodAndMode);

			int currentIdx = i - 1;

			// Downsample(Ping)
			vkCmdBindDescriptorSets(dispatchCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
				m_BloomPipeline->GetVulkanPipelineLayout(), 0, 1,
				&m_BloomPingSets[frameIndex][currentIdx], 0, nullptr);

			bloomMipSize = m_BloomTextures[0].GetMipSize(i);
			m_BloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);

			m_LodAndMode.LOD = (float)i;
			m_UBBloomLod[frameIndex].WriteandFlushBuffer(&m_LodAndMode);

			// Downsample(Pong)
			vkCmdBindDescriptorSets(dispatchCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
				m_BloomPipeline->GetVulkanPipelineLayout(), 0, 1,
				&m_BloomPongSets[frameIndex][currentIdx], 0, nullptr);

			m_BloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);
		}

		// Upsample First
		// TODO: Could have to use VkImageSubresourceRange to set correct mip level
		m_LodAndMode.LOD = float(mips - 2);
		m_LodAndMode.Mode = 2.0f;
		m_UBBloomLod[frameIndex].WriteandFlushBuffer(&m_LodAndMode);

		vkCmdBindDescriptorSets(dispatchCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			m_BloomPipeline->GetVulkanPipelineLayout(), 0, 1,
			&m_BloomUpsampleFirstSets[frameIndex], 0, nullptr);

		bloomMipSize = m_BloomTextures[2].GetMipSize(mips - 1);
		m_BloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);

		// Upsample Final
		for (int i = mips - 2; i >= 0; --i)
		{
			m_LodAndMode.LOD = (float)i;
			m_LodAndMode.Mode = 3.0f;
			m_UBBloomLod[frameIndex].WriteandFlushBuffer(&m_LodAndMode);

			vkCmdBindDescriptorSets(dispatchCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
				m_BloomPipeline->GetVulkanPipelineLayout(), 0, 1,
				&m_BloomUpsampleSets[frameIndex][i], 0, nullptr);

			bloomMipSize = m_BloomTextures[2].GetMipSize(i);
			m_BloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);
		}

	}

	void SceneRenderer::CreateCommandBuffers()
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto swapChain = VulkanSwapChain::GetSwapChain();

		m_SceneCommandBuffers.resize(swapChain->GetImageCount());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = device->GetCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(m_SceneCommandBuffers.size());

		VK_CHECK_RESULT(vkAllocateCommandBuffers(device->GetVulkanDevice(), &allocInfo, m_SceneCommandBuffers.data()), "Failed to Allocate Scene Command Buffers!");

		Renderer::SetCommandBuffers(m_SceneCommandBuffers);
	}

}