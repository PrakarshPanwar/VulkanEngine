#include "vulkanpch.h"
#include "Application.h"

#include "VulkanCore/Systems/RenderSystem.h"
#include "VulkanCore/Systems/PointLightSystem.h"
#include "Platform/Vulkan/VulkanBuffer.h"
#include "Platform/Vulkan/VulkanTexture.h"

#include "VulkanCore/Core/Log.h"
#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Camera.h"
#include "KeyboardMovementController.h"
#include <chrono>

#include "VulkanCore/Scene/Entity.h"
#include "imgui.h"

#include <numbers>
#include "imgui_impl_vulkan.h"
#include <filesystem>

namespace VulkanCore {

	Application* Application::s_Instance;

	std::shared_ptr<VulkanModel> CreateCubeModel(VulkanDevice& device, glm::vec3 offset)
	{
		ModelBuilder modelBuilder{};

		modelBuilder.Vertices = {

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

		for (auto& v : modelBuilder.Vertices)
			v.Position += offset;

		modelBuilder.Indices = { 0,  1,  2,  0,  3,  1,  4,  5,  6,  4,  7,  5,  8,  9,  10, 8,  11, 9,
						  12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21 };

		return std::make_shared<VulkanModel>(device, modelBuilder);
	}


	Application::Application()
	{
		s_Instance = this;
		Log::Init();

		std::filesystem::current_path("../VulkanCore");
		m_Window = std::make_shared<WindowsWindow>(WindowSpecs(1280, 720, "Vulkan Application"));
		m_Window->SetEventCallback(VK_CORE_BIND_EVENT_FN(Application::OnEvent));

		Init();
	}

	Application::~Application()
	{
		vkDeviceWaitIdle(m_VulkanDevice->GetVulkanDevice());
		m_ImGuiLayer->ShutDown();
	}

	void Application::Init()
	{
		m_VulkanDevice = std::make_unique<VulkanDevice>(*std::dynamic_pointer_cast<WindowsWindow>(m_Window));
		m_Renderer = std::make_unique<VulkanRenderer>(*std::dynamic_pointer_cast<WindowsWindow>(m_Window), *m_VulkanDevice);

		DescriptorPoolBuilder descriptorPoolBuilder = DescriptorPoolBuilder(*m_VulkanDevice);
		descriptorPoolBuilder.SetMaxSets(VulkanSwapChain::MaxFramesInFlight).AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VulkanSwapChain::MaxFramesInFlight);
		descriptorPoolBuilder.SetMaxSets(VulkanSwapChain::MaxFramesInFlight).AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VulkanSwapChain::MaxFramesInFlight);
		m_GlobalPool = descriptorPoolBuilder.Build();

		m_ImGuiLayer = std::make_unique<ImGuiLayer>();
		m_ImGuiLayer->OnAttach();
	}

	/*void Application::Run()
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
		descriptorSetLayoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3);
		descriptorSetLayoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3);
		descriptorSetLayoutBuilder.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3);
		auto globalSetLayout = descriptorSetLayoutBuilder.Build();

		std::vector<VkDescriptorSet> globalDescriptorSets(VulkanSwapChain::MaxFramesInFlight);
		std::vector<VulkanDescriptorWriter> vkGlobalDescriptorWriter(VulkanSwapChain::MaxFramesInFlight, { *globalSetLayout, *m_GlobalPool });

		for (int i = 0; i < globalDescriptorSets.size(); i++)
		{
			auto bufferInfo = UniformBuffers[i]->DescriptorInfo();
			vkGlobalDescriptorWriter[i].WriteBuffer(0, &bufferInfo);
			vkGlobalDescriptorWriter[i].WriteImage(1, DiffuseMaps);
			vkGlobalDescriptorWriter[i].WriteImage(2, NormalMaps);
			vkGlobalDescriptorWriter[i].WriteImage(3, SpecularMaps);

			vkGlobalDescriptorWriter[i].Build(globalDescriptorSets[i]);
		}

		RenderSystem renderSystem{ *m_VulkanDevice, m_Renderer->GetSwapChainRenderPass(), globalSetLayout->GetDescriptorSetLayout() };
		PointLightSystem pointLightSystem{ *m_VulkanDevice, m_Renderer->GetSwapChainRenderPass(), globalSetLayout->GetDescriptorSetLayout() };
		Camera GameCamera{};

		SceneInfo SceneRender{}, PointLightScene{};

		SceneRender.ScenePipeline = renderSystem.GetPipeline();
		SceneRender.PipelineLayout = renderSystem.GetPipelineLayout();

		PointLightScene.ScenePipeline = pointLightSystem.GetPipeline();
		PointLightScene.PipelineLayout = pointLightSystem.GetPipelineLayout();

		auto ImTexID = ImGui_ImplVulkan_AddTexture(DiffuseMap.GetTextureSampler(), DiffuseMap.GetVulkanImageView(), VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);

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

				m_ImGuiLayer->ImGuiBegin();
				ImGui::ShowDemoWindow((bool*)true);
				m_ImGuiLayer->ImGuiRenderandEnd(commandBuffer);

				m_Renderer->EndSwapChainRenderPass(commandBuffer);
				m_Renderer->EndFrame();
			}
		}

		vkDeviceWaitIdle(m_VulkanDevice->GetVulkanDevice());
	}*/

	void Application::RunEditor()
	{
		while (m_Running)
		{
			m_Window->OnUpdate();

			if (auto commandBuffer = m_Renderer->BeginFrame())
			{
				m_Renderer->BeginSwapChainRenderPass(commandBuffer);

				for (Layer* layer : m_LayerStack)
					layer->OnUpdate();

				m_ImGuiLayer->ImGuiBegin();
				for (Layer* layer : m_LayerStack)
					layer->OnImGuiRender();
				m_ImGuiLayer->ImGuiRenderandEnd(commandBuffer);

				m_Renderer->EndSwapChainRenderPass(commandBuffer);
				m_Renderer->EndFrame();
			}
		}
	}

	void Application::OnEvent(Event& e)
	{
		m_EditorCamera.OnEvent(e);

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(VK_CORE_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(VK_CORE_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			(*--it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::LoadEntities()
	{
		m_Scene = std::make_shared<Scene>();

		Entity CeramicVase = m_Scene->CreateEntity("Vase Model");
		CeramicVase.AddComponent<TransformComponent>(glm::vec3{ 0.0f, 0.0f, 2.5f }, glm::vec3{ 3.5f });
		CeramicVase.AddComponent<ModelComponent>(VulkanModel::CreateModelFromFile(*m_VulkanDevice, "assets/models/CeramicVase.obj", 0));

		Entity FlatPlane = m_Scene->CreateEntity("Flat Plane");
		FlatPlane.AddComponent<TransformComponent>(glm::vec3{ 0.0f, 1.6f, 0.0f }, glm::vec3{ 0.5f });
		FlatPlane.AddComponent<ModelComponent>(VulkanModel::CreateModelFromFile(*m_VulkanDevice, "assets/models/FlatPlane.obj", 2));

#define LOAD_THROUGH_ASSIMP 1

#if LOAD_THROUGH_ASSIMP
		Entity CrateModel = m_Scene->CreateEntity("Wooden Crate");
		auto& brassTransform = CrateModel.AddComponent<TransformComponent>(glm::vec3{ 0.5f, 0.0f, 4.5f }, glm::vec3{ 7.5f });
		brassTransform.Rotation = glm::vec3(-0.5f * (float)std::numbers::pi, 0.0f, 0.0f);
		CrateModel.AddComponent<ModelComponent>(VulkanModel::CreateModelFromAssimp(*m_VulkanDevice, "assets/models/WoodenCrate/WoodenCrate.gltf", 1));
		m_ModelEntity = CrateModel;
#else
		Entity BrassModel = m_Scene->CreateEntity("Brass Vase");
		auto& brassTransform = BrassModel.AddComponent<TransformComponent>(glm::vec3{ 0.5f, 0.0f, 4.5f }, glm::vec3{ 7.5f });
		brassTransform.Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		BrassModel.AddComponent<ModelComponent>(VulkanModel::CreateModelFromFile(*m_VulkanDevice, "assets/models/BrassVase.obj", 1));
		m_ModelEntity = BrassModel;
#endif

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

	bool Application::OnWindowClose(WindowCloseEvent& window)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& window)
	{
		m_Renderer->RecreateSwapChain();
		m_EditorCamera.SetAspectRatio(m_Renderer->GetAspectRatio());
		return true;
	}

}