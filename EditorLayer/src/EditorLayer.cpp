#include "EditorLayer.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Core/ImGuiLayer.h"
#include "VulkanCore/Asset/EditorAssetManager.h"
#include "VulkanCore/Asset/AssetManager.h"
#include "VulkanCore/Asset/TextureImporter.h"
#include "VulkanCore/Events/Input.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "VulkanCore/Scene/Entity.h"
#include "VulkanCore/Scene/SceneSerializer.h"
#include "VulkanCore/Utils/PlatformUtils.h"

#include <ImGuizmo.h>
#include <imgui_internal.h>
#include <optick.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace VulkanCore {

	static const std::filesystem::path g_AssetPath = "assets";

	EditorLayer::EditorLayer()
		: Layer("Editor Layer")
	{
	}

	EditorLayer::~EditorLayer()
	{
		AssetManager::Shutdown();
	}

	void EditorLayer::OnAttach()
	{
		VK_INFO("Running Editor Layer");

		std::unique_ptr<Timer> editorInit = std::make_unique<Timer>("Editor Initialization");

		m_AssetManagerBase = std::make_shared<EditorAssetManager>();
		AssetManager::Init(m_AssetManagerBase);

		m_MenuIcon = TextureImporter::LoadTexture2D("../EditorLayer/Resources/Icons/MenuIcon.png");
		m_MenuIconID = ImGuiLayer::AddTexture(*std::dynamic_pointer_cast<VulkanTexture>(m_MenuIcon));

		m_Scene = std::make_shared<Scene>();
		m_SceneRenderer = std::make_shared<SceneRenderer>(m_Scene);

		m_SceneHierarchyPanel = SceneHierarchyPanel(m_Scene);
		m_ContentBrowserPanel = std::make_unique<ContentBrowserPanel>();

		auto commandLineArgs = Application::Get()->GetSpecification().CommandLineArgs;
		if (commandLineArgs.Count > 1)
		{
			std::string sceneFilePath = commandLineArgs[1];
			OpenScene(sceneFilePath);
		}

		if (m_SceneRenderer->IsRayTraced())
			m_SceneRenderer->CreateRayTraceResources();

		m_EditorCamera = EditorCamera(glm::radians(45.0f), 1.635005f, 0.1f, 1000.0f);
	}

	void EditorLayer::OnDetach()
	{

	}

	void EditorLayer::OnUpdate()
	{
		VK_CORE_PROFILE();

		if ((m_ViewportFocused && m_ViewportHovered && !ImGuizmo::IsUsing()) || m_EditorCamera.IsInFly())
		{
			if (m_EditorCamera.OnUpdate())
				m_SceneRenderer->ResetAccumulationFrameIndex();
		}
		
		// Mouse Position relative to Viewport
		auto [mx, my] = Input::GetMousePosition();
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[0].y;
		int mouseX = (int)mx;
		int mouseY = (int)my;

		SceneEditorData sceneEditorData{};
		sceneEditorData.CameraData = m_EditorCamera;
		sceneEditorData.ViewportMousePos = glm::max(glm::ivec2{ mouseX, mouseY }, 0);
		sceneEditorData.ViewportHovered = m_ViewportHovered;
		m_SceneRenderer->SetSceneEditorData(sceneEditorData);

		if (m_SceneRenderer->IsRayTraced())
			m_SceneRenderer->TraceScene();
		else
			m_SceneRenderer->RasterizeScene();
	}

	void EditorLayer::OnEvent(Event& e)
	{
		if (!Application::Get()->GetImGuiLayer()->GetBlockEvents() || m_EditorCamera.IsInFly())
			m_EditorCamera.OnEvent(e);

		// Handling Camera Events
		if (e.Handled)
			m_SceneRenderer->ResetAccumulationFrameIndex();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(VK_CORE_BIND_EVENT_FN(EditorLayer::OnKeyEvent));
		dispatcher.Dispatch<MouseButtonPressedEvent>(VK_CORE_BIND_EVENT_FN(EditorLayer::OnMouseButtonEvent));
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

		if (m_EditorCamera.IsInFly())
			io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
		else if (io.ConfigFlags & ImGuiConfigFlags_NoMouse)
			io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				// Disabling fullscreen would allow the window to be moved to the front of other windows, 
				// which we can't undo at the moment without finer window depth/z control.
				//ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);1
				if (ImGui::MenuItem("New", "Ctrl+N"))
					NewScene();

				if (ImGui::MenuItem("Open...", "Ctrl+O"))
					OpenScene();

				if (ImGui::MenuItem("Save", "Ctrl+S"))
					SaveScene();

				if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
					SaveSceneAs();

 				//if (ImGui::MenuItem("Exit"))
 					//Application::Get()->Close();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Settings"))
			{
				ImGui::Checkbox("Show Application Stats", &m_ShowApplicationStats);
				if (ImGui::BeginMenu("Camera"))
				{
					ImGui::Text("Camera Aspect Ratio: %.6f", m_EditorCamera.GetAspectRatio());
					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		style.WindowMinSize.x = minWinSizeX;

		//ImGui::ShowDemoWindow(&m_ImGuiShowWindow);

		// Remove Padding from viewport
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));

		ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove;

		ImGui::Begin("Viewport", nullptr, viewportFlags);
		ImGui::PopStyleVar(2);

		auto region = ImGui::GetContentRegionAvail();
		m_ViewportSize = { region.x, region.y };

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImRect viewportRegion = window->WorkRect;

		auto viewportOffset = ImGui::GetCursorScreenPos();
		auto viewportMinRegion = ImVec2{ viewportRegion.Min.x - viewportOffset.x, viewportRegion.Min.y - viewportOffset.y };
		auto viewportMaxRegion = ImVec2{ viewportRegion.Max.x - viewportOffset.x, viewportRegion.Max.y - viewportOffset.y };
		m_ViewportBounds[0] = { viewportRegion.Min.x, viewportRegion.Min.y };
		m_ViewportBounds[1] = { viewportRegion.Max.x, viewportRegion.Max.y };

		if (glm::ivec2 sceneViewportSize = m_SceneRenderer->GetViewportSize();
			m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f &&
			(sceneViewportSize.x != m_ViewportSize.x || sceneViewportSize.y != m_ViewportSize.y))
		{
			m_SceneRenderer->SetViewportSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_EditorCamera.SetViewportSize(region.x, region.y);
		}

		m_ViewportHovered = ImGui::IsWindowHovered();
		m_ViewportFocused = ImGui::IsWindowFocused();
		Application::Get()->GetImGuiLayer()->BlockEvents(!m_ViewportHovered && !m_ViewportFocused);

		ImVec2 uv0 = m_SceneRenderer->IsRayTraced() ? ImVec2{ 0, 0 } : ImVec2{ 0, 1 };
		ImVec2 uv1 = m_SceneRenderer->IsRayTraced() ? ImVec2{ 1, 1 } : ImVec2{ 1, 0 };
		ImGui::SetNextItemAllowOverlap();
		ImGui::Image(m_SceneRenderer->GetSceneImage(Renderer::RT_GetCurrentFrameIndex()), region, uv0, uv1);

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const wchar_t* path = (const wchar_t*)payload->Data;
				std::filesystem::path scenePath = g_AssetPath / path;
				OpenScene(scenePath.string());
			}

			ImGui::EndDragDropTarget();
		}

		// Button Position just at the top
		ImGui::SetCursorPos({ viewportMinRegion.x + 5.0f, viewportMinRegion.y + 5.0f });
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.0f);
		if (ImGui::ImageButton("##MenuIcon", (ImTextureID)m_MenuIconID, { 20.0f, 20.0f }, { 0, 1 }, { 1, 0 }))
			ImGui::OpenPopup("EditorSettings");
		ImGui::PopStyleVar();

		if (ImGui::BeginPopup("EditorSettings"))
		{
			static float fov = 45.0f;
			static float thumbnailSize = 128.0f;
			static float padding = 16.0f;

			ImGui::Text("Camera/Content Browser");
			ImGui::DragFloat("Field of View", &fov, 0.5f, 5.0f, 90.0f);
			if (ImGui::IsItemActive())
				m_EditorCamera.SetFieldOfView(glm::radians(fov));

			ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 16, 512);
			if (ImGui::IsItemActive())
				m_ContentBrowserPanel->SetThumbnailSize(thumbnailSize);

			ImGui::SliderFloat("Padding", &padding, 0, 32);
			if (ImGui::IsItemActive())
				m_ContentBrowserPanel->SetPadding(padding);

			ImGui::Separator();
			ImGui::Text("Snap Parameters");
			ImGui::Checkbox("Snap", &m_EnableSnap);
			ImGui::DragFloat("Translation", &m_TranslationSnapValue, 0.01f, 0.01f);
			ImGui::DragFloat("Rotation", &m_RotationSnapValue);
			ImGui::DragFloat("Scale", &m_ScaleSnapValue, 0.2f);

			ImGui::EndPopup();
		}

		if (m_ViewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
			&& !ImGui::IsKeyDown(ImGuiKey_LeftAlt) && !ImGuizmo::IsOver())
		{
			int entityHandle = m_SceneRenderer->GetHoveredEntity();
			Entity hoveredEntity = entityHandle == -1 ? Entity{} : Entity{ (entt::entity)entityHandle, m_Scene.get() };

			m_SceneHierarchyPanel.SetSelectedEntity(hoveredEntity);
		}

		if (m_ShowApplicationStats)
		{
			ImGui::SetCursorPos({ viewportMinRegion.x + 50.0f, viewportMinRegion.y + 10.0f });
			SHOW_FRAMERATES;
		}

		RenderGizmo();
		ImGui::End(); // End of Viewport

		m_SceneHierarchyPanel.OnImGuiRender();
		m_ContentBrowserPanel->OnImGuiRender();
		m_SceneRenderer->OnImGuiRender();

		ImGui::End(); // End of DockSpace
	}

	bool EditorLayer::OnKeyEvent(KeyPressedEvent& e)
	{
		bool shiftKey = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);

		bool isFlying = m_EditorCamera.IsInFly();

		// Gizmos: Unreal Engine Controls
		switch (e.GetKeyCode())
		{
		case Key::Q:
		{
			if (!ImGuizmo::IsUsing())
				m_GizmoType = -1;
			break;
		}
		case Key::W:
		{
			if (!ImGuizmo::IsUsing() && !isFlying)
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
		case Key::Period:
		{
			Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
			if (selectedEntity)
			{
				auto transform = selectedEntity.GetComponent<TransformComponent>();
				m_EditorCamera.SetFocalPoint(transform.Translation);
			}

			break;
		}
		case Key::GraveAccent:
		{
			if (shiftKey)
				m_EditorCamera.SetFly(true);
			break;
		}
		}

		return false;
	}

	bool EditorLayer::OnMouseButtonEvent(MouseButtonPressedEvent& e)
	{
		switch (e.GetMouseButton())
		{
		case Mouse::ButtonLeft:
		{
			if (m_EditorCamera.IsInFly())
				m_EditorCamera.SetFly(false);
		}
		}

		return false;
	}

	bool EditorLayer::OnMouseScroll(MouseScrolledEvent& e)
	{
		return false;
	}

	bool EditorLayer::OnWindowResize(WindowResizeEvent& e)
	{
		m_WindowResized = true;
		return false;
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
			float snapValue = 0.0f; // Snap to 0.5m for translation/scale
			switch (m_GizmoType)
			{
			case ImGuizmo::TRANSLATE:
				snapValue = m_TranslationSnapValue;
				break;
			case ImGuizmo::ROTATE:
				snapValue = m_RotationSnapValue;
				break;
			case ImGuizmo::SCALE:
				snapValue = m_ScaleSnapValue;
				break;
			default:
				break;
			}

			bool snap = Input::IsKeyPressed(Key::LeftControl) || m_EnableSnap;
			float snapValues[3] = { snapValue, snapValue, snapValue };

			ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
				(ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::WORLD, glm::value_ptr(transform),
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

				if (m_SceneRenderer->IsRayTraced() && selectedEntity.HasComponent<MeshComponent>())
					m_SceneRenderer->SetUpdateTLAS();
			}
		}
	}

	void EditorLayer::NewScene()
	{
		m_Scene = std::make_shared<Scene>();
		m_SceneRenderer->SetActiveScene(m_Scene);
		m_SceneHierarchyPanel.SetContext(m_Scene);

		m_EditorScenePath = std::filesystem::path();
	}

	void EditorLayer::OpenScene()
	{
		std::string filepath = FileDialogs::OpenFile("VulkanEngine Scene (*.vkscene)\0*.vkscene\0");
		if (!filepath.empty())
			OpenScene(filepath);
	}

	void EditorLayer::OpenScene(const std::string& path)
	{
		std::filesystem::path filepath(path);

		if (filepath.extension().string() != ".vkscene")
		{
			VK_WARN("Could not load {0} - not a scene file", filepath.filename().string());
			return;
		}

		auto newScene = AssetManager::GetAsset<Scene>(path);
		if (newScene)
		{
			m_Scene = newScene;
			m_SceneRenderer->SetActiveScene(m_Scene);
			m_SceneHierarchyPanel.SetContext(m_Scene);
			m_EditorScenePath = path;
		}
	}

	void EditorLayer::SaveScene()
	{
		if (!m_EditorScenePath.empty())
			SerializeScene(m_Scene, m_EditorScenePath);
		else
			SaveSceneAs();
	}

	void EditorLayer::SaveSceneAs()
	{
		std::string filepath = FileDialogs::SaveFile("VulkanEngine Scene (*.vkscene)\0*.vkscene\0");
		if (!filepath.empty())
		{
			SerializeScene(m_Scene, filepath);
			m_EditorScenePath = filepath;
		}
	}

	void EditorLayer::SerializeScene(std::shared_ptr<Scene> scene, const std::filesystem::path& scenePath)
	{
		SceneSerializer serializer(scene);
		serializer.Serialize(scenePath.string());
	}

}