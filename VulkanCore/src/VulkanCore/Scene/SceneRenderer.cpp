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
			geomPipelineSpec.pShader = Renderer::GetShader("CorePBR");
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
		m_BloomParamsUBs.reserve(VulkanSwapChain::MaxFramesInFlight);
		m_LodUBs.reserve(VulkanSwapChain::MaxFramesInFlight);

		// Uniform Buffers
		for (int i = 0; i < VulkanSwapChain::MaxFramesInFlight; ++i)
		{
			m_UBCamera.emplace_back(sizeof(Camera));
			m_UBPointLight.emplace_back(sizeof(UBPointLights));
			m_UBSceneData.emplace_back(sizeof(SceneSettings));
#if BLOOM_COMPUTE_SHADER
			m_LodUBs.emplace_back(sizeof(LodAndMode)):
#endif
		}

#if BLOOM_COMPUTE_SHADER
		ImageSpecification imageSpec = {};
		imageSpec.Width = 1920;
		imageSpec.Height = 1080;
		imageSpec.Format = ImageFormat::RGBA16F;
		imageSpec.Usage = ImageUsage::Storage;
		m_BloomTexture = std::make_shared<VulkanImage>(imageSpec);
		m_BloomTexture->Invalidate();

		auto barrierCmd = device->GetCommandBuffer();
		Utils::InsertImageMemoryBarrier(barrierCmd, m_BloomTexture->GetVulkanImageInfo().Image,
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

		device->FlushCommandBuffer(barrierCmd);
#endif
		// Textures
		m_DiffuseMap = std::make_shared<VulkanTexture>("assets/models/CeramicVase2K/textures/antique_ceramic_vase_01_diff_2k.jpg");
		m_NormalMap = std::make_shared<VulkanTexture>("assets/models/CeramicVase2K/textures/antique_ceramic_vase_01_nor_gl_2k.png", ImageFormat::RGBA8_UNORM);
		m_ARMMap = std::make_shared<VulkanTexture>("assets/models/CeramicVase2K/textures/antique_ceramic_vase_01_arm_2k.png", ImageFormat::RGBA8_UNORM);

		m_DiffuseMap2 = std::make_shared<VulkanTexture>("assets/models/BrassVase2K/textures/brass_vase_03_diff_2k.jpg");
		m_NormalMap2 = std::make_shared<VulkanTexture>("assets/models/BrassVase2K/textures/brass_vase_03_nor_gl_2k.png", ImageFormat::RGBA8_UNORM);
		m_ARMMap2 = std::make_shared<VulkanTexture>("assets/models/BrassVase2K/textures/brass_vase_03_arm_2k.png", ImageFormat::RGBA8_UNORM);

		m_DiffuseMap3 = std::make_shared<VulkanTexture>("assets/textures/StoneTiles/StoneTilesDiff.png");
		m_NormalMap3 = std::make_shared<VulkanTexture>("assets/textures/StoneTiles/StoneTilesNorGL.png", ImageFormat::RGBA8_UNORM);
		m_ARMMap3 = std::make_shared<VulkanTexture>("assets/textures/StoneTiles/StoneTilesARM.png", ImageFormat::RGBA8_UNORM);

#if USE_PRELOADED_BRDF
		m_BRDFTexture = std::make_shared<VulkanTexture>("assets/textures/BRDF_LUTMap.png", ImageFormat::RGBA8_UNORM);
#else
		m_BRDFTexture = VulkanRenderer::CreateBRDFTexture();
#endif

		auto [filteredMap, irradianceMap] = VulkanRenderer::CreateEnviromentMap("assets/cubemaps/HDR/Birchwood4K.hdr");
		m_CubemapTexture = filteredMap;
		m_IrradianceTexture = irradianceMap;

		m_SkyboxMesh = Utils::CreateCubeModel();

		std::vector<VkDescriptorImageInfo> DiffuseMaps, ARMMaps, NormalMaps;
		DiffuseMaps.push_back(m_DiffuseMap->GetDescriptorImageInfo());
		DiffuseMaps.push_back(m_DiffuseMap2->GetDescriptorImageInfo());
		DiffuseMaps.push_back(m_DiffuseMap3->GetDescriptorImageInfo());
		NormalMaps.push_back(m_NormalMap->GetDescriptorImageInfo());
		NormalMaps.push_back(m_NormalMap2->GetDescriptorImageInfo());
		NormalMaps.push_back(m_NormalMap3->GetDescriptorImageInfo());
		ARMMaps.push_back(m_ARMMap->GetDescriptorImageInfo());
		ARMMaps.push_back(m_ARMMap2->GetDescriptorImageInfo());
		ARMMaps.push_back(m_ARMMap3->GetDescriptorImageInfo());

		// Writing in Descriptors
		auto vulkanDescriptorPool = Application::Get()->GetDescriptorPool();

		m_GeometryDescriptorSets.resize(VulkanSwapChain::MaxFramesInFlight);
		m_PointLightDescriptorSets.resize(VulkanSwapChain::MaxFramesInFlight);
		m_CompositeDescriptorSets.resize(VulkanSwapChain::MaxFramesInFlight);
		m_BloomDescriptorSets.resize(VulkanSwapChain::MaxFramesInFlight);
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
			geomDescriptorWriter[i].WriteImage(4, ARMMaps);
			
			// Irradiance Map
			VkDescriptorImageInfo irradianceMapInfo = m_IrradianceTexture->GetDescriptorImageInfo();
			geomDescriptorWriter[i].WriteImage(5, &irradianceMapInfo);

			// BRDF LUT Texture
#if USE_PRELOADED_BRDF
			VkDescriptorImageInfo brdfTextureInfo = m_BRDFTexture->GetDescriptorImageInfo();
#else
			VkDescriptorImageInfo brdfTextureInfo = m_BRDFTexture.GetDescriptorInfo();
#endif
			geomDescriptorWriter[i].WriteImage(6, &brdfTextureInfo);

			// Prefiltered Map
			VkDescriptorImageInfo prefilteredMapInfo = filteredMap->GetDescriptorImageInfo();
			geomDescriptorWriter[i].WriteImage(7, &prefilteredMapInfo);

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
		// Compute Demo Descriptors
		std::vector<VulkanDescriptorWriter> computeDemoDescriptorWriter(
			VulkanSwapChain::MaxFramesInFlight,
			{ *m_BloomPipeline->GetDescriptorSetLayout(), *vulkanDescriptorPool });

		for (int i = 0; i < m_BloomDescriptorSets.size(); ++i)
		{
			auto outputImageInfo = m_BloomTexture->GetDescriptorInfo();
			computeDemoDescriptorWriter[i].WriteImage(0, &outputImageInfo);

			VkDescriptorImageInfo imagesInfo = m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetResolveAttachment()[i].GetDescriptorInfo();
			computeDemoDescriptorWriter[i].WriteImage(1, &imagesInfo);

			auto lodUBInfo = m_LodUBs[i].GetDescriptorBufferInfo();
			computeDemoDescriptorWriter[i].WriteBuffer(2, &lodUBInfo);

			bool success = computeDemoDescriptorWriter[i].Build(m_BloomDescriptorSets[i]);
			VK_CORE_ASSERT(success, "Failed to Write to Descriptor Set!");
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

#if BLOOM_COMPUTE_SHADER
			VkDescriptorImageInfo imagesInfo = m_BloomTexture->GetDescriptorInfo();
#else
			VkDescriptorImageInfo imagesInfo = m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetResolveAttachment()[i].GetDescriptorInfo();
#endif
			compDescriptorWriter[i].WriteImage(0, &imagesInfo);

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
		ImGui::DragFloat("Skybox LOD", &m_SkyboxSettings.LOD, 0.01f, 0.0f, 11.0f);
		ImGui::DragFloat("Skybox Intensity", &m_SkyboxSettings.Intensity, 0.01f, 0.0f, 20.0f);

		if (ImGui::TreeNode("Scene Renderer Stats##GPUPerf"))
		{
			Renderer::RetrieveQueryPoolResults();

			ImGui::Text("Geometry Pass: %lluns", Renderer::GetQueryTime(0));
			ImGui::Text("Skybox Pass: %lluns", Renderer::GetQueryTime(2));
			ImGui::Text("Composite Pass: %lluns", Renderer::GetQueryTime(3));
			ImGui::TreePop();
		}

		ImGui::End();
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
		m_LodUBs[frameIndex].WriteandFlushBuffer(&m_LodAndMode);
#endif

		GeometryPass();
#if BLOOM_COMPUTE_SHADER
		BloomBlurPass();
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

		Renderer::BeginGPUPerfMarker();

		// Rendering Skybox
		Renderer::RenderSkybox(m_SkyboxPipeline, m_SkyboxMesh, m_SkyboxDescriptorSets, &m_SkyboxSettings);
		Renderer::EndGPUPerfMarker();

		// Rendering Point Lights
		Renderer::BeginGPUPerfMarker();
		m_Scene->OnUpdateLights(m_SceneCommandBuffers, m_PointLightPipeline, m_PointLightDescriptorSets);
		Renderer::EndGPUPerfMarker();

		Renderer::EndRenderPass(m_GeometryPipeline->GetSpecification().RenderPass);
	}

	void SceneRenderer::BloomBlurPass()
	{
		int frameIndex = Renderer::GetCurrentFrameIndex();

		auto dispatchCmd = m_SceneCommandBuffers[frameIndex];
		m_BloomPipeline->Bind(dispatchCmd);

		vkCmdBindDescriptorSets(dispatchCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			m_BloomPipeline->GetVulkanPipelineLayout(), 0, 1,
			&m_BloomDescriptorSets[frameIndex], 0, nullptr);

		m_BloomPipeline->Dispatch(dispatchCmd, 128, 128, 1);
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