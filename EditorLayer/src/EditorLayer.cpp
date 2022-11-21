#include "EditorLayer.h"

#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"
#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/ImGuiLayer.h"
#include "VulkanCore/Events/Input.h"
#include "VulkanCore/Scene/Entity.h"
#include "VulkanCore/Renderer/VulkanRenderer.h"
#include "VulkanCore/Renderer/Renderer.h"

#include "Platform/Vulkan/VulkanMesh.h"
#include "Platform/Vulkan/VulkanSwapChain.h"
#include "Platform/Vulkan/VulkanContext.h"

#include <imgui_impl_vulkan.h>
#include <ImGuizmo.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "VulkanCore/Renderer/Renderer.h"

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
		VK_INFO("Running Editor Layer");

		std::unique_ptr<Timer> editorInit = std::make_unique<Timer>("Editor Initialization");

		LoadEntities();
		m_SceneRenderer = std::make_shared<SceneRenderer>(m_Scene);

		m_SceneImages.resize(VulkanSwapChain::MaxFramesInFlight);

		for (int i = 0; i < VulkanSwapChain::MaxFramesInFlight; i++)
			m_SceneImages[i] = ImGuiLayer::AddTexture(m_SceneRenderer->GetFinalPassImage(i));

		m_SceneHierarchyPanel = SceneHierarchyPanel(m_Scene);

		m_EditorCamera = EditorCamera(glm::radians(45.0f), 1.635005f, 0.1f, 100.0f);
	}

	void EditorLayer::OnDetach()
	{

	}

	void EditorLayer::OnUpdate()
	{
		m_EditorCamera.OnUpdate();
		m_SceneRenderer->RenderScene(m_EditorCamera);
	}

	void EditorLayer::OnEvent(Event& e)
	{
		if (!Application::Get()->GetImGuiLayer()->GetBlockEvents())
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

		// TODO: Shift these operations in Renderer
		constexpr std::array<uint64_t, 2> queryPoolBuffer = { 0, 0 };
		vkGetQueryPoolResults(VulkanContext::GetCurrentDevice()->GetVulkanDevice(),
			VulkanRenderer::Get()->GetPerfQueryPool(),
			0, 2, sizeof(uint64_t) * 2,
			(void*)queryPoolBuffer.data(), sizeof(uint64_t),
			VK_QUERY_RESULT_64_BIT);

		uint64_t timeStamp = queryPoolBuffer[1] - queryPoolBuffer[0];

		std::chrono::duration rasterTime = 
			std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(timeStamp));

		//ImGui::ShowDemoWindow(&m_ImGuiShowWindow);
		ImGui::Begin("Application Stats");
		SHOW_FRAMERATES;
		ImGui::Checkbox("Show ImGui Demo Window", &m_ImGuiShowWindow);
		ImGui::Text("Scene Rasterization Time: %llums", rasterTime.count());
		ImGui::Text("Camera Aspect Ratio: %.6f", m_EditorCamera.GetAspectRatio());
		ImGui::End();

		ImGui::Begin("Viewport");
		auto region = ImGui::GetContentRegionAvail();
		auto windowSize = ImGui::GetWindowSize();
		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();
		m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

		if ((m_ViewportSize.x != region.x) && (m_ViewportSize.y != region.y))
		{
			VK_TRACE("Viewport has been Resized!");
			m_ViewportSize = region;
			m_SceneRenderer->SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
			RecreateSceneDescriptors();
			m_EditorCamera.SetViewportSize(region.x, region.y);
		}

		m_ViewportHovered = ImGui::IsWindowHovered();
		m_ViewportFocused = ImGui::IsWindowFocused();
		Application::Get()->GetImGuiLayer()->BlockEvents(!m_ViewportHovered && !m_ViewportFocused);

		ImGui::Image(m_SceneImages[Renderer::GetCurrentFrameIndex()], region, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		RenderGizmo();
		ImGui::End(); // End of Viewport

		m_SceneHierarchyPanel.OnImGuiRender();
		m_SceneRenderer->OnImGuiRender();

		ImGui::End(); // End of DockSpace
	}

	bool EditorLayer::OnKeyEvent(KeyPressedEvent& keyevent)
	{
		// Gizmos: Unreal Engine Controls
		switch (keyevent.GetKeyCode())
		{
		case Key::Q:
		{
			if (!ImGuizmo::IsUsing())
				m_GizmoType = -1;
			break;
		}
		case Key::W:
		{
			if (!ImGuizmo::IsUsing())
				m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
			break;
		}
		case Key::E:
		{
			if (!ImGuizmo::IsUsing())
				m_GizmoType = ImGuizmo::OPERATION::ROTATE;
			break;
		}
		case Key::R:
		{
			if (!ImGuizmo::IsUsing())
				m_GizmoType = ImGuizmo::OPERATION::SCALE;
			break;
		}
		}
		return false;
	}

	bool EditorLayer::OnMouseScroll(MouseScrolledEvent& mouseScroll)
	{
		return false;
	}

	bool EditorLayer::OnWindowResize(WindowResizeEvent& windowEvent)
	{
		m_WindowResized = true;
		return false;
	}

	void EditorLayer::RecreateSceneDescriptors()
	{
		m_SceneImages.clear();
		m_SceneImages.resize(VulkanSwapChain::MaxFramesInFlight);

		for (int i = 0; i < VulkanSwapChain::MaxFramesInFlight; i++)
			m_SceneImages[i] = ImGuiLayer::AddTexture(m_SceneRenderer->GetFinalPassImage(i));
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
		std::shared_ptr<PointLight> blueLight = std::make_shared<PointLight>(glm::vec4(blueLightTransform.Translation, 1.0f), glm::vec4{ 0.2f, 0.3f, 0.8f, 1.0f });
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

	void EditorLayer::RenderGizmo()
	{
		Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();

		if (selectedEntity && m_GizmoType != -1)
		{
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();

			ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y, m_ViewportBounds[1].x - m_ViewportBounds[0].x, m_ViewportBounds[1].y - m_ViewportBounds[0].y);
		
			// Editor camera
			const glm::mat4& cameraProjection = m_EditorCamera.GetProjectionMatrix();
			glm::mat4 cameraView = m_EditorCamera.GetViewMatrix();

			// Entity transform
			auto& tc = selectedEntity.GetComponent<TransformComponent>();
			glm::mat4 transform = tc.GetTransform();

			// Snapping
			bool snap = Input::IsKeyPressed(Key::LeftControl);
			float snapValue = 0.5f; // Snap to 0.5m for translation/scale
			// Snap to 45 degrees for rotation
			if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
				snapValue = 45.0f;

			float snapValues[3] = { snapValue, snapValue, snapValue };

			ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
				(ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),
				nullptr, snap ? snapValues : nullptr);

			if (ImGuizmo::IsUsing())
			{
				glm::vec3 translation, scale, skew;
				glm::vec4 perspective;
				glm::quat rotation;

				glm::decompose(transform, scale, rotation, translation, skew, perspective);

				tc.Translation = translation;
				tc.Rotation = glm::eulerAngles(rotation);
				tc.Scale = scale;
			}
		}
	}

}