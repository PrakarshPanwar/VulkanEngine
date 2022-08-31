#include "EditorLayer.h"

#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Core/Log.h"
#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Scene/Entity.h"
#include "VulkanCore/Renderer/VulkanRenderer.h"

#include "Platform/Vulkan/VulkanModel.h"
#include "Platform/Vulkan/VulkanSwapChain.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include <numbers>
#include <filesystem>

#define SHOW_FRAMERATES ImGui::Text("Application Stats:\n\t Frame Time: %.3f ms\n\t Frames Per Second: %.2f FPS", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate)
#define USE_IMGUI_VIEWPORTS 0

namespace VulkanCore {

	EditorLayer::EditorLayer()
		: Layer("Editor Layer")
	{
	}

	EditorLayer::~EditorLayer()
	{
	}

	void EditorLayer::OnAttach()
	{
		VK_CORE_INFO("Running Editor Layer");
		LoadEntities();

		VulkanDevice& vulkanDevice = *VulkanDevice::GetDevice();
		VulkanRenderer* vkRenderer = VulkanRenderer::Get();

		for (auto& UniformBuffer : m_UniformBuffers)
		{
			UniformBuffer = std::make_unique<VulkanBuffer>(vulkanDevice, sizeof(UniformBufferDataComponent), 1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			UniformBuffer->Map();
		}

#define TEXTURE_SYSTEM 1

		m_DiffuseMap = std::make_shared<VulkanTexture>("assets/textures/PlainSnow/SnowDiffuse.jpg");
		m_NormalMap = std::make_shared<VulkanTexture>("assets/textures/PlainSnow/SnowNormalGL.png");
		m_SpecularMap = std::make_shared<VulkanTexture>("assets/textures/PlainSnow/SnowSpecular.jpg");

		m_DiffuseMap2 = std::make_shared<VulkanTexture>("assets/textures/DeformedSnow/SnowDiffuse.jpg");
		m_NormalMap2 = std::make_shared<VulkanTexture>("assets/textures/DeformedSnow/SnowNormalGL.png");
		m_SpecularMap2 = std::make_shared<VulkanTexture>("assets/textures/DeformedSnow/SnowSpecular.jpg");

		m_DiffuseMap3 = std::make_shared<VulkanTexture>("assets/textures/Marble/MarbleDiff.png");
		m_NormalMap3 = std::make_shared<VulkanTexture>("assets/textures/Marble/MarbleNormalGL.png");
		m_SpecularMap3 = std::make_shared<VulkanTexture>("assets/textures/Marble/MarbleSpec.jpg");

#if USE_IMGUI_VIEWPORTS
		m_SwapChainImage = std::make_shared<VulkanTexture>(VulkanSwapChain::GetSwapChain()->GetSwapChainImage(0),
			VulkanSwapChain::GetSwapChain()->GetImageView(0));
		m_SwapChainTexID = ImGui_ImplVulkan_AddTexture(m_SwapChainImage->GetTextureSampler(),
			m_SwapChainImage->GetVulkanImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
#endif

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

		DescriptorSetLayoutBuilder descriptorSetLayoutBuilder = DescriptorSetLayoutBuilder(vulkanDevice);
		descriptorSetLayoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS);
		descriptorSetLayoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3);
		descriptorSetLayoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3);
		descriptorSetLayoutBuilder.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3);
		auto globalSetLayout = descriptorSetLayoutBuilder.Build();

		std::vector<VulkanDescriptorWriter> vkGlobalDescriptorWriter(VulkanSwapChain::MaxFramesInFlight,
			{ *globalSetLayout, *Application::Get()->GetVulkanDescriptorPool() });

		for (int i = 0; i < m_GlobalDescriptorSets.size(); i++)
		{
			auto bufferInfo = m_UniformBuffers[i]->DescriptorInfo();
			vkGlobalDescriptorWriter[i].WriteBuffer(0, &bufferInfo);
			vkGlobalDescriptorWriter[i].WriteImage(1, DiffuseMaps);
			vkGlobalDescriptorWriter[i].WriteImage(2, NormalMaps);
			vkGlobalDescriptorWriter[i].WriteImage(3, SpecularMaps);

			vkGlobalDescriptorWriter[i].Build(m_GlobalDescriptorSets[i]);
		}

		m_SceneHierarchyPanel = SceneHierarchyPanel(m_Scene);
		m_RenderSystem = std::make_shared<RenderSystem>(vulkanDevice, vkRenderer->GetSwapChainRenderPass(), globalSetLayout->GetDescriptorSetLayout());
		m_PointLightSystem = std::make_shared<PointLightSystem>(vulkanDevice, vkRenderer->GetSwapChainRenderPass(), globalSetLayout->GetDescriptorSetLayout());

		m_SceneRender.ScenePipeline = m_RenderSystem->GetPipeline();
		m_SceneRender.PipelineLayout = m_RenderSystem->GetPipelineLayout();

		m_PointLightScene.ScenePipeline = m_PointLightSystem->GetPipeline();
		m_PointLightScene.PipelineLayout = m_PointLightSystem->GetPipelineLayout();

		m_EditorCamera = EditorCamera(glm::radians(50.0f), vkRenderer->GetAspectRatio(), 0.1f, 100.0f);
	}

	void EditorLayer::OnDetach()
	{

	}

	void EditorLayer::OnUpdate()
	{
		m_EditorCamera.OnUpdate();

		int frameIndex = VulkanRenderer::Get()->GetFrameIndex();

		m_SceneRender.SceneDescriptorSet = m_GlobalDescriptorSets[frameIndex];
		m_SceneRender.CommandBuffer = VulkanRenderer::Get()->GetCurrentCommandBuffer();

		m_PointLightScene.SceneDescriptorSet = m_GlobalDescriptorSets[frameIndex];
		m_PointLightScene.CommandBuffer = VulkanRenderer::Get()->GetCurrentCommandBuffer();

		UniformBufferDataComponent uniformBuffer{};
		uniformBuffer.Projection = m_EditorCamera.GetProjectionMatrix();
		uniformBuffer.View = m_EditorCamera.GetViewMatrix();
		uniformBuffer.InverseView = glm::inverse(m_EditorCamera.GetViewMatrix());
		m_Scene->UpdateUniformBuffer(uniformBuffer);
		m_UniformBuffers[frameIndex]->WriteToBuffer(&uniformBuffer);
		m_UniformBuffers[frameIndex]->FlushBuffer();

		m_Scene->OnUpdate(m_SceneRender);
		m_Scene->OnUpdateLights(m_PointLightScene);
	}

	void EditorLayer::OnEvent(Event& e)
	{
		if (!Application::Get()->GetImGuiLayer()->GetBlockEvents())
			m_EditorCamera.OnEvent(e);

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(VK_CORE_BIND_EVENT_FN(EditorLayer::OnKeyEvent));
	}

	void EditorLayer::OnImGuiRender()
	{
		ImGui::ShowDemoWindow(&m_ImGuiShowWindow);

		ImGui::Begin("Application Stats");
		SHOW_FRAMERATES;
		ImGui::Checkbox("Show ImGui Demo Window", &m_ImGuiShowWindow);
		ImGui::End();

#if USE_IMGUI_VIEWPORTS
		ImGui::Begin("Viewport");
		ImGui::Image(m_SwapChainTexID, { 400.0f, 400.0f });
		ImGui::End();
#endif

		m_SceneHierarchyPanel.OnImGuiRender();
	}

	bool EditorLayer::OnKeyEvent(KeyPressedEvent& keyevent)
	{
		return false;
	}

	bool EditorLayer::OnMouseScroll(MouseScrolledEvent& mouseScroll)
	{
		return false;
	}

	void EditorLayer::LoadEntities()
	{
		m_Scene = std::make_shared<Scene>();

		Entity CeramicVase = m_Scene->CreateEntity("Vase Model");
		CeramicVase.AddComponent<TransformComponent>(glm::vec3{ 0.0f, 0.0f, 2.5f }, glm::vec3{ 3.5f });
		CeramicVase.AddComponent<ModelComponent>(VulkanModel::CreateModelFromFile(*VulkanDevice::GetDevice(), "assets/models/CeramicVase.obj", 0));

		Entity FlatPlane = m_Scene->CreateEntity("Flat Plane");
		FlatPlane.AddComponent<TransformComponent>(glm::vec3{ 0.0f, 1.6f, 0.0f }, glm::vec3{ 0.5f });
		FlatPlane.AddComponent<ModelComponent>(VulkanModel::CreateModelFromFile(*VulkanDevice::GetDevice(), "assets/models/FlatPlane.obj", 2));

		Entity CrateModel = m_Scene->CreateEntity("Wooden Crate");
		auto& crateTransform = CrateModel.AddComponent<TransformComponent>(glm::vec3{ 0.5f, 0.0f, 4.5f }, glm::vec3{ 1.5f });
		crateTransform.Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		CrateModel.AddComponent<ModelComponent>(VulkanModel::CreateModelFromAssimp(*VulkanDevice::GetDevice(), "assets/models/WoodenCrate/WoodenCrate.gltf", 1));

		Entity BrassVase = m_Scene->CreateEntity("Brass Vase");
		auto& brassTransform = BrassVase.AddComponent<TransformComponent>(glm::vec3{ 1.5f, 0.0f, 1.5f }, glm::vec3{ 6.0f });
		brassTransform.Rotation = glm::vec3(-0.5f * std::numbers::pi_v<float>, 0.0f, 0.0f);
		BrassVase.AddComponent<ModelComponent>(VulkanModel::CreateModelFromAssimp(*VulkanDevice::GetDevice(), "assets/models/BrassVase2K/BrassVase.fbx", 0));

		Entity BluePointLight = m_Scene->CreateEntity("Blue Light");
		auto& blueLightTransform = BluePointLight.AddComponent<TransformComponent>(glm::vec3{ -1.0f, 0.0f, 4.5f }, glm::vec3{ 0.1f });
		std::shared_ptr<PointLight> blueLight = std::make_shared<PointLight>(glm::vec4(blueLightTransform.Translation, 1.0f), glm::vec4{ 0.2f, 0.3f, 0.8f, 2.0f });
		BluePointLight.AddComponent<PointLightComponent>(blueLight);

		Entity RedPointLight = m_Scene->CreateEntity("Red Light");
		auto& redLightTransform = RedPointLight.AddComponent<TransformComponent>(glm::vec3{ 1.5f, 0.0f, 5.0f }, glm::vec3{ 0.1f });
		std::shared_ptr<PointLight> redLight = std::make_shared<PointLight>(glm::vec4(redLightTransform.Translation, 1.0f), glm::vec4{ 1.0f, 0.5f, 0.1f, 1.0f });
		RedPointLight.AddComponent<PointLightComponent>(redLight);

		Entity GreenPointLight = m_Scene->CreateEntity("Green Light");
		auto& greenLightTransform = GreenPointLight.AddComponent<TransformComponent>(glm::vec3{ 2.0f, 0.0f, -0.5f }, glm::vec3{ 0.1f });
		std::shared_ptr<PointLight> greenLight = std::make_shared<PointLight>(glm::vec4(greenLightTransform.Translation, 1.0f), glm::vec4{ 0.1f, 0.8f, 0.2f, 1.0f });
		GreenPointLight.AddComponent<PointLightComponent>(greenLight);
	}

}