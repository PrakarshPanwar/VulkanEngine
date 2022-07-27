#include "vulkanpch.h"
#include "VulkanApplication.h"

#include "Systems/RenderSystem.h"
#include "Systems/PointLightSystem.h"
#include "VulkanBuffer.h"
#include "Core/VulkanTexture.h"

#include "Core/Log.h"
#include "Core/Assert.h"
#include "Core/Core.h"
#include "Renderer/Camera.h"
#include "KeyboardMovementController.h"
#include <chrono>

#include "Scene/Entity.h"
#include "imgui.h"

namespace VulkanCore {

	VulkanApplication* VulkanApplication::s_Instance;

	std::shared_ptr<VulkanModel> CreateCubeModel(VulkanDevice& device, glm::vec3 offset)
	{
		ModelBuilder modelBuilder{};

		modelBuilder.vertices = {

			// Left Face
			{ { -0.5f, -0.5f, -0.5f }, { 0.9f, 0.9f, 0.9f } },
			{ { -0.5f,  0.5f,  0.5f }, { 0.9f, 0.9f, 0.9f } },
			{ { -0.5f, -0.5f,  0.5f }, { 0.9f, 0.9f, 0.9f } },
			{ { -0.5f,  0.5f, -0.5f }, { 0.9f, 0.9f, 0.9f } },

			// Right Face (Yellow)
			{ {  0.5f, -0.5f, -0.5f }, { 0.8f, 0.8f, 0.1f } },
			{ {  0.5f,  0.5f,  0.5f }, { 0.8f, 0.8f, 0.1f } },
			{ {  0.5f, -0.5f,  0.5f }, { 0.8f, 0.8f, 0.1f } },
			{ {  0.5f,  0.5f, -0.5f }, { 0.8f, 0.8f, 0.1f } },

			// Top Face (Orange, Remember Y-Axis points down)
			{ { -0.5f, -0.5f, -0.5f }, { 0.9f, 0.6f, 0.1f } },
			{ {  0.5f, -0.5f,  0.5f }, { 0.9f, 0.6f, 0.1f } },
			{ { -0.5f, -0.5f,  0.5f }, { 0.9f, 0.6f, 0.1f } },
			{ {  0.5f, -0.5f, -0.5f }, { 0.9f, 0.6f, 0.1f } },

			// Bottom Face (Red)
			{ { -0.5f,  0.5f, -0.5f }, { 0.8f, 0.1f, 0.1f } },
			{ {  0.5f,  0.5f,  0.5f }, { 0.8f, 0.1f, 0.1f } },
			{ { -0.5f,  0.5f,  0.5f }, { 0.8f, 0.1f, 0.1f } },
			{ {  0.5f,  0.5f, -0.5f }, { 0.8f, 0.1f, 0.1f } },

			// Nose Face (Blue)
			{ { -0.5f, -0.5f,  0.5f }, { 0.1f, 0.1f, 0.8f } },
			{ {  0.5f,  0.5f,  0.5f }, { 0.1f, 0.1f, 0.8f } },
			{ { -0.5f,  0.5f,  0.5f }, { 0.1f, 0.1f, 0.8f } },
			{ {  0.5f, -0.5f,  0.5f }, { 0.1f, 0.1f, 0.8f } },

			// Tail Face (green)
			{ { -0.5f, -0.5f, -0.5f }, { 0.1f, 0.8f, 0.1f } },
			{ {  0.5f,  0.5f, -0.5f }, { 0.1f, 0.8f, 0.1f } },
			{ { -0.5f,  0.5f, -0.5f }, { 0.1f, 0.8f, 0.1f } },
			{ {  0.5f, -0.5f, -0.5f }, { 0.1f, 0.8f, 0.1f } }
		};

		for (auto& v : modelBuilder.vertices)
			v.Position += offset;

		modelBuilder.indices = { 0,  1,  2,  0,  3,  1,  4,  5,  6,  4,  7,  5,  8,  9,  10, 8,  11, 9,
						  12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21 };

		return std::make_shared<VulkanModel>(device, modelBuilder);
	}


	VulkanApplication::VulkanApplication()
	{
		s_Instance = this;
		m_Window = std::make_unique<WindowsWindow>(WindowSpecs(1280, 720, "Vulkan Application"));
		m_Window->SetEventCallback(VK_CORE_BIND_EVENT_FN(VulkanApplication::OnEvent));
		Init();
	}

	VulkanApplication::~VulkanApplication()
	{
	}

	void VulkanApplication::Init()
	{
		m_VulkanDevice = std::make_unique<VulkanDevice>(*m_Window);
		m_Renderer = std::make_unique<VulkanRenderer>(*m_Window, *m_VulkanDevice);

		DescriptorPoolBuilder descriptorPoolBuilder = DescriptorPoolBuilder(*m_VulkanDevice);
		descriptorPoolBuilder.SetMaxSets(VulkanSwapChain::MaxFramesInFlight).AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VulkanSwapChain::MaxFramesInFlight);
		descriptorPoolBuilder.SetMaxSets(VulkanSwapChain::MaxFramesInFlight).AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VulkanSwapChain::MaxFramesInFlight);
		m_GlobalPool = descriptorPoolBuilder.Build();

		m_ImGuiLayer = std::make_unique<ImGuiLayer>();
		LoadEntities();
	}

	void VulkanApplication::Run()
	{
		std::vector<std::unique_ptr<VulkanBuffer>> UniformBuffers(VulkanSwapChain::MaxFramesInFlight);

		for (auto& UniformBuffer : UniformBuffers)
		{
			UniformBuffer = std::make_unique<VulkanBuffer>(*m_VulkanDevice, sizeof(UniformBufferDataComponent), 1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			UniformBuffer->Map();
		}

#define TEXTURE_SYSTEM 1

		VulkanTexture DiffuseMap(*m_VulkanDevice, "assets/textures/PlainSnow/SnowDiffuse.jpg");
		VulkanTexture NormalMap(*m_VulkanDevice, "assets/textures/PlainSnow/SnowNormalGL.png");
		VulkanTexture SpecularMap(*m_VulkanDevice, "assets/textures/PlainSnow/SnowSpecular.jpg");

		VulkanTexture DiffuseMap2(*m_VulkanDevice, "assets/textures/DeformedSnow/SnowDiffuse.jpg");
		VulkanTexture NormalMap2(*m_VulkanDevice, "assets/textures/DeformedSnow/SnowNormalGL.png");
		VulkanTexture SpecularMap2(*m_VulkanDevice, "assets/textures/DeformedSnow/SnowSpecular.jpg");

		VulkanTexture DiffuseMap3(*m_VulkanDevice, "assets/textures/Marble/MarbleDiff.png");
		VulkanTexture NormalMap3(*m_VulkanDevice, "assets/textures/Marble/MarbleNormalGL.png");
		VulkanTexture SpecularMap3(*m_VulkanDevice, "assets/textures/Marble/MarbleSpec.jpg");

		//m_ImGuiLayer->OnAttach();

		std::vector<VkDescriptorImageInfo> DiffuseMaps, SpecularMaps, NormalMaps;
		DiffuseMaps.push_back(DiffuseMap.GetDescriptorImageInfo());
		DiffuseMaps.push_back(DiffuseMap2.GetDescriptorImageInfo());
		DiffuseMaps.push_back(DiffuseMap3.GetDescriptorImageInfo());
		NormalMaps.push_back(NormalMap.GetDescriptorImageInfo());
		NormalMaps.push_back(NormalMap2.GetDescriptorImageInfo());
		NormalMaps.push_back(NormalMap3.GetDescriptorImageInfo());
		SpecularMaps.push_back(SpecularMap.GetDescriptorImageInfo());
		SpecularMaps.push_back(SpecularMap2.GetDescriptorImageInfo());
		SpecularMaps.push_back(SpecularMap3.GetDescriptorImageInfo());

		DescriptorSetLayoutBuilder descriptorSetLayoutBuilder = DescriptorSetLayoutBuilder(*m_VulkanDevice);
		descriptorSetLayoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS);
		descriptorSetLayoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 8);
		descriptorSetLayoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 8);
		descriptorSetLayoutBuilder.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 8);
		auto globalSetLayout = descriptorSetLayoutBuilder.Build();

		std::vector<VkDescriptorSet> globalDescriptorSets(VulkanSwapChain::MaxFramesInFlight);

		for (int i = 0; i < globalDescriptorSets.size(); i++)
		{
			auto bufferInfo = UniformBuffers[i]->DescriptorInfo();

			VulkanDescriptorWriter vkDescriptorWriter(*globalSetLayout, *m_GlobalPool);
			vkDescriptorWriter.WriteBuffer(0, &bufferInfo);
			vkDescriptorWriter.WriteImage(1, DiffuseMaps);
			vkDescriptorWriter.WriteImage(2, NormalMaps);
			vkDescriptorWriter.WriteImage(3, SpecularMaps);

			vkDescriptorWriter.Build(globalDescriptorSets[i]);
		}

		RenderSystem renderSystem{ *m_VulkanDevice, m_Renderer->GetSwapChainRenderPass(), globalSetLayout->GetDescriptorSetLayout() };
		PointLightSystem pointLightSystem{ *m_VulkanDevice, m_Renderer->GetSwapChainRenderPass(), globalSetLayout->GetDescriptorSetLayout() };
		Camera GameCamera{};

		SceneInfo SceneRender{}, PointLightScene{};

		SceneRender.ScenePipeline = renderSystem.GetPipeline();
		SceneRender.PipelineLayout = renderSystem.GetPipelineLayout();

		PointLightScene.ScenePipeline = pointLightSystem.GetPipeline();
		PointLightScene.PipelineLayout = pointLightSystem.GetPipelineLayout();

		m_EditorCamera = EditorCamera(glm::radians(50.0f), m_Renderer->GetAspectRatio(), 0.1f, 100.0f);

		while (m_Running)
		{
			m_Window->OnUpdate();
			m_EditorCamera.OnUpdate();

			if (auto commandBuffer = m_Renderer->BeginFrame())
			{
				int frameIndex = m_Renderer->GetFrameIndex();

				SceneRender.SceneDescriptorSet = globalDescriptorSets[frameIndex];
				SceneRender.CommandBuffer = commandBuffer;

				PointLightScene.SceneDescriptorSet = globalDescriptorSets[frameIndex];
				PointLightScene.CommandBuffer = commandBuffer;

				UniformBufferDataComponent uniformBuffer{};
				uniformBuffer.Projection = m_EditorCamera.GetProjectionMatrix();
				uniformBuffer.View = m_EditorCamera.GetViewMatrix();
				uniformBuffer.InverseView = glm::inverse(m_EditorCamera.GetViewMatrix());
				m_Scene->UpdateUniformBuffer(uniformBuffer);
				UniformBuffers[frameIndex]->WriteToBuffer(&uniformBuffer);
				UniformBuffers[frameIndex]->FlushBuffer();

				m_Renderer->BeginSwapChainRenderPass(commandBuffer);
				m_Scene->OnUpdate(SceneRender);
				m_Scene->OnUpdateLights(PointLightScene);
				m_Renderer->EndSwapChainRenderPass(commandBuffer);
				m_Renderer->EndFrame();

				/*m_ImGuiLayer->ImGuiBegin();
				ImGui::ShowDemoWindow((bool*)true);
				m_ImGuiLayer->ImGuiRender();
				m_ImGuiLayer->ImGuiEnd(commandBuffer);*/
			}
		}

		vkDeviceWaitIdle(m_VulkanDevice->GetVulkanDevice());
	}

	void VulkanApplication::OnEvent(Event& e)
	{
		m_EditorCamera.OnEvent(e);

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(VK_CORE_BIND_EVENT_FN(VulkanApplication::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(VK_CORE_BIND_EVENT_FN(VulkanApplication::OnWindowResize));
	}

	void VulkanApplication::LoadGameObjects()
	{
		std::shared_ptr<VulkanModel> VaseModel = VulkanModel::CreateModelFromFile(*m_VulkanDevice, "assets/models/CeramicVase.obj", 0);

		auto Vase = VulkanGameObject::CreateGameObject();
		Vase.Model = VaseModel;
		Vase.Transform.Translation = { 0.0f, 0.0f, 2.5f };
		Vase.Transform.Scale = glm::vec3{ 3.5f };
		Vase.Color = { 0.2f, 0.3f, 0.8f };
		m_GameObjects.emplace(Vase.GetObjectID(), std::move(Vase));

		std::shared_ptr<VulkanModel> FlatPlaneModel = VulkanModel::CreateModelFromFile(*m_VulkanDevice, "assets/models/FlatPlane.obj", 1);

		auto FlatPlane = VulkanGameObject::CreateGameObject();
		FlatPlane.Model = FlatPlaneModel;
		FlatPlane.Transform.Translation = { 0.0f, 1.6f, 0.0f };
		FlatPlane.Transform.Scale = glm::vec3{ 0.5f };
		m_GameObjects.emplace(FlatPlane.GetObjectID(), std::move(FlatPlane));

		std::vector<glm::vec3> LightColors{
			{ 0.1f, 0.8f, 0.2f },
			{ 0.2f, 0.3f, 0.8f },
			{ 1.0f, 0.5f, 0.1f }
		};

		auto GreenPointLight = VulkanGameObject::MakePointLight(0.7f, 0.1f, LightColors[0]);
		GreenPointLight.Transform.Translation = { 2.0f, 0.0f, -0.5f };
		m_GameObjects.emplace(GreenPointLight.GetObjectID(), std::move(GreenPointLight));

		auto BluePointLight = VulkanGameObject::MakePointLight(0.5f, 0.1f, LightColors[1]);
		BluePointLight.Transform.Translation = { -1.0f, 0.0f, 4.5f };
		m_GameObjects.emplace(BluePointLight.GetObjectID(), std::move(BluePointLight));

		auto RedPointLight = VulkanGameObject::MakePointLight(0.8f, 0.1f, LightColors[2]);
		RedPointLight.Transform.Translation = { 1.5f, 0.0f, 5.0f };
		m_GameObjects.emplace(RedPointLight.GetObjectID(), std::move(RedPointLight));
	}

	void VulkanApplication::LoadEntities()
	{
		m_Scene = std::make_shared<Scene>();

		Entity CeramicVase = m_Scene->CreateEntity("Vase Model");
		CeramicVase.AddComponent<TransformComponent>(glm::vec3{ 0.0f, 0.0f, 2.5f }, glm::vec3{ 3.5f });
		CeramicVase.AddComponent<ModelComponent>(VulkanModel::CreateModelFromFile(*m_VulkanDevice, "assets/models/CeramicVase.obj", 0));

		Entity FlatPlane = m_Scene->CreateEntity("Flat Plane Model");
		FlatPlane.AddComponent<TransformComponent>(glm::vec3{ 0.0f, 1.6f, 0.0f }, glm::vec3{ 0.5f });
		FlatPlane.AddComponent<ModelComponent>(VulkanModel::CreateModelFromFile(*m_VulkanDevice, "assets/models/FlatPlane.obj", 2));

		Entity BrassVase = m_Scene->CreateEntity("Brass Vase Model");
		BrassVase.AddComponent<TransformComponent>(glm::vec3{ 0.5f, 0.0f, 4.5f }, glm::vec3{ 7.5f });
		BrassVase.AddComponent<ModelComponent>(VulkanModel::CreateModelFromFile(*m_VulkanDevice, "assets/models/BrassVase.obj", 1));

		Entity CubeModel = m_Scene->CreateEntity("Basic Cube Model");
		CubeModel.AddComponent<TransformComponent>(glm::vec3{ 1.5f, 0.0f, -5.5f }, glm::vec3{ 1.5f });
		CubeModel.AddComponent<ModelComponent>(CreateCubeModel(*m_VulkanDevice, { 2.5f, 0.0f, 5.5f }));

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

	bool VulkanApplication::OnWindowClose(WindowCloseEvent& window)
	{
		m_Running = false;
		return true;
	}

	bool VulkanApplication::OnWindowResize(WindowResizeEvent& window)
	{
		m_Renderer->RecreateSwapChain();
		m_EditorCamera.SetAspectRatio(m_Renderer->GetAspectRatio());
		return true;
	}

}