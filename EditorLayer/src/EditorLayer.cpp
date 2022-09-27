#include "EditorLayer.h"

#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Core/Log.h"
#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Scene/Entity.h"
#include "VulkanCore/Renderer/VulkanRenderer.h"

#include "Platform/Vulkan/VulkanMesh.h"
#include "Platform/Vulkan/VulkanSwapChain.h"

#include <imgui_impl_vulkan.h>

#include <memory>
#include <filesystem>
#include <numbers>
#include <future>

#define SHOW_FRAMERATES ImGui::Text("Application Stats:\n\t Frame Time: %.3f ms\n\t Frames Per Second: %.2f FPS", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate)
#define USE_IMGUI_VIEWPORTS 0

namespace VulkanCore {

	EditorLayer::EditorLayer()
		: Layer("Editor Layer")
	{
	}

	EditorLayer::~EditorLayer()
	{
		m_SceneImages.clear();
	}

	void EditorLayer::OnAttach()
	{
		VK_CORE_INFO("Running Editor Layer");

		std::unique_ptr<Timer> editorInit = std::make_unique<Timer>("Editor Initialization");

		m_SceneRenderer = std::make_shared<SceneRenderer>();
		LoadEntities();

		VulkanDevice& device = *VulkanDevice::GetDevice();
		VulkanRenderer* vkRenderer = VulkanRenderer::Get();

		for (auto& UniformBuffer : m_UniformBuffers)
		{
			UniformBuffer = std::make_unique<VulkanBuffer>(sizeof(UniformBufferDataComponent), 1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			UniformBuffer->Map();
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

		m_SceneImages.reserve(VulkanSwapChain::MaxFramesInFlight);
		m_SceneTextureIDs.resize(VulkanSwapChain::MaxFramesInFlight);

		for (int i = 0; i < VulkanSwapChain::MaxFramesInFlight; i++)
		{
			m_SceneImages.emplace_back(m_SceneRenderer->GetImage(i), m_SceneRenderer->GetImageView(i), false);

			m_SceneTextureIDs[i] = ImGui_ImplVulkan_AddTexture(
				m_SceneImages[i].GetTextureSampler(),
				m_SceneImages[i].GetVulkanImageView(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

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

		DescriptorSetLayoutBuilder descriptorSetLayoutBuilder = DescriptorSetLayoutBuilder(device);
		descriptorSetLayoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS);
		descriptorSetLayoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3);
		descriptorSetLayoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3);
		descriptorSetLayoutBuilder.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3);
		descriptorSetLayoutBuilder.AddBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT);
		auto globalSetLayout = descriptorSetLayoutBuilder.Build();

		std::vector<VulkanDescriptorWriter> vkGlobalDescriptorWriter(VulkanSwapChain::MaxFramesInFlight,
			{ *globalSetLayout, *Application::Get()->GetVulkanDescriptorPool() });

#define USE_VULKAN_IMAGE 0
#if USE_VULKAN_IMAGE
		ImageSpecification spec;
		spec.Width = 1280;
		spec.Height = 720;
		spec.Usage = ImageUsage::Storage;
		spec.Format = ImageFormat::RGBA16F;

		m_TextureImage = std::make_shared<VulkanImage>(spec);
		m_TextureImage->Invalidate();

		std::vector<VkDescriptorImageInfo> newImgs;
		newImgs.emplace_back(m_TextureImage->GetDescriptorInfo());

		auto cmdBuffer = device.GetCommandBuffer();

		Utils::InsertImageMemoryBarrier(cmdBuffer, m_TextureImage->GetVulkanImageInfo(),
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

		device.FlushCommandBuffer(cmdBuffer);
#endif

		for (int i = 0; i < m_GlobalDescriptorSets.size(); i++)
		{
			auto bufferInfo = m_UniformBuffers[i]->DescriptorInfo();
			vkGlobalDescriptorWriter[i].WriteBuffer(0, &bufferInfo);
			vkGlobalDescriptorWriter[i].WriteImage(1, DiffuseMaps);
			vkGlobalDescriptorWriter[i].WriteImage(2, NormalMaps);
			vkGlobalDescriptorWriter[i].WriteImage(3, SpecularMaps);
#if USE_VULKAN_IMAGE
			vkGlobalDescriptorWriter[i].WriteImage(4, newImgs);
#endif

			vkGlobalDescriptorWriter[i].Build(m_GlobalDescriptorSets[i]);
		}

		auto RetRenderSystem = [&]()
		{
			return std::make_shared<RenderSystem>(m_SceneRenderer->GetRenderPass(), globalSetLayout->GetDescriptorSetLayout());
		};

		auto RetPointLightSystem = [&]()
		{
			return std::make_shared<PointLightSystem>(m_SceneRenderer->GetRenderPass(), globalSetLayout->GetDescriptorSetLayout());
		};

		m_SceneHierarchyPanel = SceneHierarchyPanel(m_Scene);

#define PIPELINE_MULTITHREADED 1

#if PIPELINE_MULTITHREADED
		auto renderSystemThread = std::async(std::launch::async, RetRenderSystem);
		auto pointLightSystemThread = std::async(std::launch::async, RetPointLightSystem);

		m_RenderSystem = renderSystemThread.get();
		m_PointLightSystem = pointLightSystemThread.get();
#else
		m_RenderSystem = std::make_shared<RenderSystem>(m_SceneRenderer->GetRenderPass(), globalSetLayout->GetDescriptorSetLayout());
		m_PointLightSystem = std::make_shared<PointLightSystem>(m_SceneRenderer->GetRenderPass(), globalSetLayout->GetDescriptorSetLayout());
#endif

		m_SceneRender.ScenePipeline = m_RenderSystem->GetPipeline();
		m_SceneRender.PipelineLayout = m_RenderSystem->GetPipelineLayout();

		m_PointLightScene.ScenePipeline = m_PointLightSystem->GetPipeline();
		m_PointLightScene.PipelineLayout = m_PointLightSystem->GetPipelineLayout();

		//m_EditorCamera = EditorCamera(glm::radians(45.0f), vkRenderer->GetAspectRatio(), 0.1f, 100.0f);
		m_EditorCamera = EditorCamera(glm::radians(45.0f), 1.635005f, 0.1f, 100.0f);
	}

	void EditorLayer::OnDetach()
	{

	}

	void EditorLayer::OnUpdate()
	{
		m_EditorCamera.OnUpdate();

		int frameIndex = VulkanRenderer::Get()->GetFrameIndex();

		m_SceneRender.SceneDescriptorSet = m_GlobalDescriptorSets[frameIndex];
		m_SceneRender.CommandBuffer = m_SceneRenderer->GetCommandBuffer(frameIndex);

		m_PointLightScene.SceneDescriptorSet = m_GlobalDescriptorSets[frameIndex];
		m_PointLightScene.CommandBuffer = m_SceneRenderer->GetCommandBuffer(frameIndex);

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
		if (!Application::Get()->GetImGuiLayerPtr()->GetBlockEvents())
			m_EditorCamera.OnEvent(e);

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(VK_CORE_BIND_EVENT_FN(EditorLayer::OnKeyEvent));
		dispatcher.Dispatch<WindowResizeEvent>(VK_CORE_BIND_EVENT_FN(EditorLayer::OnWindowResize));
	}

	void EditorLayer::OnImGuiRender()
	{
		// Note: Switch this to true to enable dockspace
		static bool dockspaceOpen = true;
		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Window", &dockspaceOpen, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		style.WindowMinSize.x = minWinSizeX;

		//ImGui::ShowDemoWindow(&m_ImGuiShowWindow);
		ImGui::Begin("Application Stats");
		SHOW_FRAMERATES;
		ImGui::Checkbox("Show ImGui Demo Window", &m_ImGuiShowWindow);
		ImGui::ArrowButton("ArrowButton", ImGuiDir_Right);
		ImGui::Text("Camera Aspect Ratio: %.6f", m_EditorCamera.GetAspectRatio());
		ImGui::End();

		ImGui::Begin("Viewport");
		auto region = ImGui::GetContentRegionAvail();
		auto windowSize = ImGui::GetWindowSize();

		if ((m_ViewportSize.x != region.x) && (m_ViewportSize.y != region.y))
		{
			VK_CORE_TRACE("Viewport has been Resized!");
			m_ViewportSize = region;
			RecreateSceneDescriptors();
			m_EditorCamera.SetViewportSize(region.x, region.y);
		}

		m_ViewportHovered = ImGui::IsWindowHovered();
		m_ViewportFocused = ImGui::IsWindowFocused();
		Application::Get()->GetImGuiLayerPtr()->BlockEvents(!m_ViewportHovered && !m_ViewportFocused);

		ImGui::Image(m_SceneTextureIDs[VulkanRenderer::Get()->GetFrameIndex()], region);
		ImGui::End(); // End of Viewport

		m_SceneHierarchyPanel.OnImGuiRender();

		ImGui::End(); // End of DockSpace
	}

	bool EditorLayer::OnKeyEvent(KeyPressedEvent& keyevent)
	{
		return false;
	}

	bool EditorLayer::OnMouseScroll(MouseScrolledEvent& mouseScroll)
	{
		return false;
	}

	bool EditorLayer::OnWindowResize(WindowResizeEvent& windowEvent)
	{
		m_WindowResized = true;
		m_SceneRenderer->RecreateScene();
		return true;
	}

	void EditorLayer::RecreateSceneDescriptors()
	{
		m_SceneImages.clear();
		m_SceneTextureIDs.clear();

		m_SceneImages.reserve(VulkanSwapChain::MaxFramesInFlight);
		m_SceneTextureIDs.resize(VulkanSwapChain::MaxFramesInFlight);

		for (int i = 0; i < VulkanSwapChain::MaxFramesInFlight; i++)
		{
			m_SceneImages.emplace_back(m_SceneRenderer->GetImage(i), m_SceneRenderer->GetImageView(i), false);

			m_SceneTextureIDs[i] = ImGui_ImplVulkan_AddTexture(
				m_SceneImages[i].GetTextureSampler(),
				m_SceneImages[i].GetVulkanImageView(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	}

	void EditorLayer::LoadEntities()
	{
		m_Scene = std::make_shared<Scene>();

		Entity CeramicVase = m_Scene->CreateEntity("Ceramic Vase");
		CeramicVase.AddComponent<TransformComponent>(glm::vec3{ 0.0f, -1.2f, 2.5f }, glm::vec3{ 3.5f });
		CeramicVase.AddComponent<MeshComponent>(VulkanMesh::CreateMeshFromAssimp("assets/models/CeramicVase2K/antique_ceramic_vase_01_2k.obj", 0));

		Entity FlatPlane = m_Scene->CreateEntity("Flat Plane");
		FlatPlane.AddComponent<TransformComponent>(glm::vec3{ 0.0f, -1.3f, 0.0f }, glm::vec3{ 0.5f });
		FlatPlane.AddComponent<MeshComponent>(VulkanMesh::CreateMeshFromFile("assets/models/FlatPlane.obj", 2));

		Entity CrateModel = m_Scene->CreateEntity("Wooden Crate");
		auto& crateTransform = CrateModel.AddComponent<TransformComponent>(glm::vec3{ 0.5f, 0.0f, 4.5f }, glm::vec3{ 1.5f });
		crateTransform.Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		CrateModel.AddComponent<MeshComponent>(VulkanMesh::CreateMeshFromAssimp("assets/models/WoodenCrate/WoodenCrate.gltf", 1));

		Entity BrassVase = m_Scene->CreateEntity("Brass Vase");
		auto& brassTransform = BrassVase.AddComponent<TransformComponent>(glm::vec3{ 1.5f, 0.0f, 1.5f }, glm::vec3{ 6.0f });
		brassTransform.Rotation = glm::vec3(glm::radians(90.0f), 0.0f, 0.0f);
		BrassVase.AddComponent<MeshComponent>(VulkanMesh::CreateMeshFromAssimp("assets/models/BrassVase2K/BrassVase.fbx", 1));

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