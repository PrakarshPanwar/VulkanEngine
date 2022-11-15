#include "vulkanpch.h"
#include "SceneRenderer.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"
#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Core/Timer.h"
#include "VulkanCore/Renderer/Renderer.h"

#include "Platform/Vulkan/VulkanMesh.h"
#include "Platform/Vulkan/VulkanAllocator.h"

namespace VulkanCore {

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
#if USE_SCENE_RENDERER_TO_DRAW
		CreatePipelines();
		CreateDescriptorSets();
#else
		CreateRenderPass();
#endif
	}

	void SceneRenderer::CreateRenderPass()
	{
		std::unique_ptr<Timer> timer = std::make_unique<Timer>("Render Pass Creation");

		FramebufferSpecification fbSpec;
		fbSpec.Width = 1920;
		fbSpec.Height = 1080;
		fbSpec.Attachments = { ImageFormat::RGBA8_SRGB, ImageFormat::DEPTH24STENCIL8 };
		fbSpec.ClearColor = { 0.5f, 0.1f, 0.7f, 1.0f };
		fbSpec.Samples = 8;

		m_ViewportSize = { fbSpec.Width, fbSpec.Height };

		m_SceneFramebuffer = std::make_shared<VulkanFramebuffer>(fbSpec);

		RenderPassSpecification rpSpec;
		rpSpec.TargetFramebuffer = m_SceneFramebuffer;

		m_SceneRenderPass = std::make_shared<VulkanRenderPass>(rpSpec);
	}

	void SceneRenderer::CreatePipelines()
	{
		// Geometry and Point Light Pipeline
		{
			FramebufferSpecification geomFramebufferSpec;
			geomFramebufferSpec.Width = 1920;
			geomFramebufferSpec.Height = 1080;
			geomFramebufferSpec.Attachments = { ImageFormat::RGBA8_SRGB, ImageFormat::DEPTH24STENCIL8 };
			geomFramebufferSpec.Samples = 8;

			RenderPassSpecification geomRenderPassSpec;
			geomRenderPassSpec.TargetFramebuffer = std::make_shared<VulkanFramebuffer>(geomFramebufferSpec);
			m_SceneFramebuffer = geomRenderPassSpec.TargetFramebuffer;

			PipelineSpecification geomPipelineSpec;
			geomPipelineSpec.pShader = Renderer::GetShader("CoreShader");
			geomPipelineSpec.RenderPass = std::make_shared<VulkanRenderPass>(geomRenderPassSpec);
			geomPipelineSpec.Layout = { Vertex::GetBindingDescriptions(), Vertex::GetAttributeDescriptions() };

			PipelineSpecification pointLightPipelineSpec;
			pointLightPipelineSpec.pShader = Renderer::GetShader("PointLightShader");
			pointLightPipelineSpec.Blend = true;
			pointLightPipelineSpec.RenderPass = geomPipelineSpec.RenderPass;

			m_GeometryPipeline = std::make_shared<VulkanPipeline>(geomPipelineSpec);
			m_PointLightPipeline = std::make_shared<VulkanPipeline>(pointLightPipelineSpec);
		}
	}

	void SceneRenderer::CreateDescriptorSets()
	{
		for (int i = 0; i < m_CameraUBs.size(); ++i)
		{
			auto& CameraUB = m_CameraUBs.at(i);
			CameraUB = std::make_unique<VulkanBuffer>(sizeof(UBCamera), 1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			CameraUB->Map();

			auto& PointLightUB = m_PointLightUBs.at(i);
			PointLightUB = std::make_unique<VulkanBuffer>(sizeof(UBPointLights), 1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			PointLightUB->Map();
		}

		m_DiffuseMap = std::make_shared<VulkanTexture>("assets/models/CeramicVase2K/textures/antique_ceramic_vase_01_diff_2k.jpg");
		m_NormalMap = std::make_shared<VulkanTexture>("assets/models/CeramicVase2K/textures/antique_ceramic_vase_01_nor_gl_2k.jpg");
		m_SpecularMap = std::make_shared<VulkanTexture>("assets/textures/PlainSnow/SnowSpecular.jpg");

		m_DiffuseMap2 = std::make_shared<VulkanTexture>("assets/textures/DeformedSnow/SnowDiffuse.jpg");
		m_NormalMap2 = std::make_shared<VulkanTexture>("assets/textures/DeformedSnow/SnowNormalGL.png");
		m_SpecularMap2 = std::make_shared<VulkanTexture>("assets/textures/DeformedSnow/SnowSpecular.jpg");

		m_DiffuseMap3 = std::make_shared<VulkanTexture>("assets/textures/Marble/MarbleDiff.png");
		m_NormalMap3 = std::make_shared<VulkanTexture>("assets/textures/Marble/MarbleNormalGL.png");
		m_SpecularMap3 = std::make_shared<VulkanTexture>("assets/textures/Marble/MarbleSpec.jpg");

		// TODO: Shift these operations to SceneRenderer
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

		DescriptorSetLayoutBuilder descriptorSetLayoutBuilder = DescriptorSetLayoutBuilder();
		descriptorSetLayoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		auto pointLightDescriptorSetLayout = descriptorSetLayoutBuilder.Build();

		descriptorSetLayoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		descriptorSetLayoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3);
		descriptorSetLayoutBuilder.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3);
		descriptorSetLayoutBuilder.AddBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3);
		auto sceneDescriptorSetLayout = descriptorSetLayoutBuilder.Build();

		auto vulkanDescriptorPool = Application::Get()->GetDescriptorPool();

		m_GeometryDescriptorSets.resize(VulkanSwapChain::MaxFramesInFlight);
		m_PointLightDescriptorSets.resize(VulkanSwapChain::MaxFramesInFlight);

		std::vector<VulkanDescriptorWriter> sceneDescriptorWriter(
			VulkanSwapChain::MaxFramesInFlight,
			{ *sceneDescriptorSetLayout, *vulkanDescriptorPool });

		for (int i = 0; i < m_GeometryDescriptorSets.size(); i++)
		{
			auto cameraUBInfo = m_CameraUBs[i]->DescriptorInfo();
			sceneDescriptorWriter[i].WriteBuffer(0, &cameraUBInfo);

			auto pointLightUBInfo = m_PointLightUBs[i]->DescriptorInfo();
			sceneDescriptorWriter[i].WriteBuffer(1, &pointLightUBInfo);

			sceneDescriptorWriter[i].WriteImage(2, DiffuseMaps);
			sceneDescriptorWriter[i].WriteImage(3, NormalMaps);
			sceneDescriptorWriter[i].WriteImage(4, SpecularMaps);

			sceneDescriptorWriter[i].Build(m_GeometryDescriptorSets[i]);
		}

		std::vector<VulkanDescriptorWriter> pointLightDescriptorWriter(
			VulkanSwapChain::MaxFramesInFlight,
			{ *pointLightDescriptorSetLayout, *vulkanDescriptorPool });

		for (int i = 0; i < m_PointLightDescriptorSets.size(); i++)
		{
			auto cameraUBInfo = m_CameraUBs[i]->DescriptorInfo();
			pointLightDescriptorWriter[i].WriteBuffer(0, &cameraUBInfo);

			bool success = pointLightDescriptorWriter[i].Build(m_PointLightDescriptorSets[i]);
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

	void SceneRenderer::RenderScene(EditorCamera& camera)
	{
		Renderer::BeginRenderPass(m_GeometryPipeline->GetSpecification().RenderPass);

		int frameIndex = Renderer::GetCurrentFrameIndex();

		UBCamera cameraUB{};
		cameraUB.Projection = camera.GetProjectionMatrix();
		cameraUB.View = camera.GetViewMatrix();
		cameraUB.InverseView = glm::inverse(camera.GetViewMatrix());
		m_CameraUBs[frameIndex]->WriteToBuffer(&cameraUB);
		m_CameraUBs[frameIndex]->FlushBuffer();

		UBPointLights pointLightUB{};
		m_Scene->UpdatePointLightUB(pointLightUB);
		m_PointLightUBs[frameIndex]->WriteToBuffer(&pointLightUB);
		m_PointLightUBs[frameIndex]->FlushBuffer();

		m_Scene->OnUpdateGeometry(m_SceneCommandBuffers, m_GeometryPipeline, m_GeometryDescriptorSets);
		m_Scene->OnUpdateLights(m_SceneCommandBuffers, m_PointLightPipeline, m_PointLightDescriptorSets);

		Renderer::EndRenderPass(m_GeometryPipeline->GetSpecification().RenderPass);
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