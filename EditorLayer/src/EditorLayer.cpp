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

		// TODO: Move this to Icon class
		m_MenuIcon = TextureImporter::LoadTexture2D("../../EditorLayer/Resources/Icons/MenuIcon.png");
		m_PlayIcon = TextureImporter::LoadTexture2D("../../EditorLayer/Resources/Icons/PlayIcon.png");
		m_PauseIcon = TextureImporter::LoadTexture2D("../../EditorLayer/Resources/Icons/PauseIcon.png");
		m_StepIcon = TextureImporter::LoadTexture2D("../../EditorLayer/Resources/Icons/StepIcon.png");
		m_SimulateIcon = TextureImporter::LoadTexture2D("../../EditorLayer/Resources/Icons/SimulateIcon.png");
		m_StopIcon = TextureImporter::LoadTexture2D("../../EditorLayer/Resources/Icons/StopIcon.png");

		m_MenuIconID = ImGuiLayer::AddTexture(*std::dynamic_pointer_cast<VulkanTexture>(m_MenuIcon));
		m_PlayIconID = ImGuiLayer::AddTexture(*std::dynamic_pointer_cast<VulkanTexture>(m_PlayIcon));
		m_PauseIconID = ImGuiLayer::AddTexture(*std::dynamic_pointer_cast<VulkanTexture>(m_PauseIcon));
		m_StepIconID = ImGuiLayer::AddTexture(*std::dynamic_pointer_cast<VulkanTexture>(m_StepIcon));
		m_SimulateIconID = ImGuiLayer::AddTexture(*std::dynamic_pointer_cast<VulkanTexture>(m_SimulateIcon));
		m_StopIconID = ImGuiLayer::AddTexture(*std::dynamic_pointer_cast<VulkanTexture>(m_StopIcon));

		m_EditorScene = std::make_shared<Scene>();
		m_ActiveScene = m_EditorScene;
		m_SceneRenderer = std::make_shared<SceneRenderer>(m_ActiveScene);

		m_SceneHierarchyPanel = SceneHierarchyPanel(m_ActiveScene);
		m_ContentBrowserPanel = std::make_unique<ContentBrowserPanel>();

		auto commandLineArgs = Application::Get()->GetSpecification().CommandLineArgs;
		if (commandLineArgs.Count > 1)
		{
			std::string sceneFilePath = commandLineArgs[1];
			OpenScene(sceneFilePath);
		}

		m_EditorCamera = EditorCamera(glm::radians(45.0f), 1.635005f, 0.1f, 1000.0f);
	}

	void EditorLayer::OnDetach()
	{

	}

	void EditorLayer::OnUpdate()
	{
		VK_CORE_PROFILE();

		if ((m_ViewportFocused && m_ViewportHovered && !ImGuizmo::IsUsing()) || m_EditorCamera.IsInFly())
			m_EditorCamera.OnUpdate();

		switch (m_SceneState)
		{
		case SceneState::Edit:
		{
			m_EditorCamera.OnUpdate();
			break;
		}
		case SceneState::Simulate:
		{
			m_EditorCamera.OnUpdate();
			m_ActiveScene->OnUpdateSimulation();
			break;
		}
		case SceneState::Play:
		{
			m_ActiveScene->OnUpdateRuntime();
			break;
		}
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

		m_SceneRenderer->RenderScene();
	}

	void EditorLayer::OnEvent(Event& e)
	{
		if (!Application::Get()->GetImGuiLayer()->GetBlockEvents() || m_EditorCamera.IsInFly())
			m_EditorCamera.OnEvent(e);

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
 				//	Application::Get()->Close();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Settings"))
			{
				ImGui::Checkbox("Show Application Stats", &m_ShowApplicationStats);
				ImGui::Checkbox("Show Camera Data", &m_ShowCameraData);

				ImGui::EndMenu();
			}

			if (std::popcount(m_TransformInputMask))
			{
				auto selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
				if (selectedEntity)
				{
					ImGui::PushItemWidth(45.0f);
					ImGui::SetKeyboardFocusHere();
					bool edited = ImGui::InputFloat("##TransformInput", &m_TransformScalarInput, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_CharsDecimal);
					ImGui::PopItemWidth();

					ImGui::TextColored(m_TransformInputMask & 0x1 ? ImVec4{ 0.8f, 0.0f, 0.0f, 1.0f } : ImVec4{ 0.1f, 0.1f, 0.1f, 1.0f }, "X = %.3f", m_TransformInput.x);
					ImGui::SameLine();
					ImGui::TextColored(m_TransformInputMask & 0x2 ? ImVec4{ 0.0f, 0.8f, 0.0f, 1.0f } : ImVec4{ 0.1f, 0.1f, 0.1f, 1.0f }, "\tY = %.3f", m_TransformInput.y);
					ImGui::SameLine();
					ImGui::TextColored(m_TransformInputMask & 0x4 ? ImVec4{ 0.0f, 0.0f, 0.8f, 1.0f } : ImVec4{ 0.1f, 0.1f, 0.1f, 1.0f }, "\tZ = %.3f", m_TransformInput.z);

					if (edited || ImGui::IsKeyPressed(ImGuiKey_X) || ImGui::IsKeyPressed(ImGuiKey_Y) || ImGui::IsKeyPressed(ImGuiKey_Z))
					{
						// Transfer Input Data using bit flag
						__m128 value = _mm_mask_mov_ps(_mm_setzero_ps(), m_TransformInputMask, _mm_set1_ps(m_TransformScalarInput));
						_mm_store_ps(glm::value_ptr(m_TransformInput), value);

						auto& transform = selectedEntity.GetComponent<TransformComponent>();
						transform = m_EntityTransform;

						switch (m_GizmoType)
						{
						case ImGuizmo::OPERATION::TRANSLATE:
						{
							transform.Translation += glm::vec3(m_TransformInput);
							break;
						}
						case ImGuizmo::OPERATION::ROTATE:
						{
							transform.Rotation += glm::vec3(glm::radians(m_TransformInput));
							break;
						}
						case ImGuizmo::OPERATION::SCALE:
						{
							__m128 scale = _mm_mask_mov_ps(_mm_set1_ps(1.0f), m_TransformInputMask, value);
							_mm_store_ps(glm::value_ptr(m_TransformInput), scale);

							transform.Scale *= glm::vec3(m_TransformInput);
							break;
						}
						default:
							break;
						}
					}

					if (ImGui::IsKeyPressed(ImGuiKey_Enter))
						m_TransformInputMask = 0;
				}
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

		ImGui::SetNextItemAllowOverlap();
		ImGui::Image(m_SceneRenderer->GetSceneImage(Renderer::RT_GetCurrentFrameIndex()), region, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				std::filesystem::path scenePath = (const wchar_t*)payload->Data;
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
			Entity hoveredEntity = entityHandle == -1 ? Entity{} : Entity{ (entt::entity)entityHandle, m_ActiveScene.get() };

			m_SceneHierarchyPanel.SetSelectedEntity(hoveredEntity);
		}

		if (m_ShowApplicationStats)
		{
			ImGui::SetCursorPos({ viewportMinRegion.x + 50.0f, viewportMinRegion.y + 10.0f });
			SHOW_FRAMERATES;
		}

		if (m_ShowCameraData)
		{
			ImGui::SetCursorPos({ viewportMaxRegion.x - 350.0f, viewportMinRegion.y + 10.0f });

			glm::vec3 cameraDirection = m_EditorCamera.GetForwardDirection();
			ImGui::Text("Aspect Ratio: %.2f\t Direction: %.3f, %.3f, %.3f", m_EditorCamera.GetAspectRatio(), cameraDirection.x, cameraDirection.y, cameraDirection.z);
		}

		RenderGizmo();
		ImGui::End(); // End of Viewport

		UI_Toolbar();
		m_SceneHierarchyPanel.OnImGuiRender();
		m_ContentBrowserPanel->OnImGuiRender();
		m_SceneRenderer->OnImGuiRender();

		ImGui::End(); // End of DockSpace
	}

	bool EditorLayer::OnKeyEvent(KeyPressedEvent& e)
	{
		bool shiftKey = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
		bool altKey = Input::IsKeyPressed(Key::LeftAlt) || Input::IsKeyPressed(Key::RightAlt);

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

			if (shiftKey)
			{
				Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
				m_TransformInputMask = !m_EditorCamera.IsInFly() && selectedEntity ? 0x7 : 0;
				m_EntityTransform = m_TransformInputMask ? selectedEntity.GetComponent<TransformComponent>() : TransformComponent{};
				m_TransformInput = {};
			}

			break;
		}
		case Key::E:
		{
			if (!ImGuizmo::IsUsing())
				m_GizmoType = ImGuizmo::OPERATION::ROTATE;

			if (shiftKey)
			{
				Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
				m_TransformInputMask = !m_EditorCamera.IsInFly() && selectedEntity ? 0x7 : 0;
				m_EntityTransform = m_TransformInputMask ? selectedEntity.GetComponent<TransformComponent>() : TransformComponent{};
				m_TransformInput = {};
			}

			break;
		}
		case Key::R:
		{
			if (!ImGuizmo::IsUsing())
				m_GizmoType = ImGuizmo::OPERATION::SCALE;

			if (shiftKey)
			{
				Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
				m_TransformInputMask = !m_EditorCamera.IsInFly() && selectedEntity ? 0x7 : 0;
				m_EntityTransform = m_TransformInputMask ? selectedEntity.GetComponent<TransformComponent>() : TransformComponent{};
				m_TransformInput = glm::vec4{ 1.0f };
			}

			break;
		}
		case Key::X:
		{
			if (m_TransformInputMask)
				m_TransformInputMask = 0x1;

			break;
		}
		case Key::Y:
		{
			if (m_TransformInputMask)
				m_TransformInputMask = 0x2;

			break;
		}
		case Key::Z:
		{
			if (m_TransformInputMask)
				m_TransformInputMask = 0x4;

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

			if (selectedEntity.HasComponent<TransformComponent>())
			{
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
				}
			}
			/*else if (selectedEntity.HasComponent<DirectionalLightComponent>())
			{
				auto drlc = selectedEntity.GetComponent<DirectionalLightComponent>();
				glm::mat4 rotation = glm::toMat4(glm::quat(drlc.Direction));
			}*/
		}
	}

	void EditorLayer::UI_Toolbar()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		auto& colors = ImGui::GetStyle().Colors;
		const auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
		const auto& buttonActive = colors[ImGuiCol_ButtonActive];
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

		ImGui::Begin("##Toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		bool toolbarEnabled = (bool)m_ActiveScene;

		ImVec4 tintColor = ImVec4(1, 1, 1, 1);
		if (!toolbarEnabled)
			tintColor.w = 0.5f;

		float size = ImGui::GetWindowHeight() - 4.0f;
		ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));

		bool hasPlayButton = m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play;
		bool hasSimulateButton = m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate;
		bool hasPauseButton = m_SceneState != SceneState::Edit;

		if (hasPlayButton)
		{
			auto icon = (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate) ? m_PlayIconID : m_StopIconID;
			if (ImGui::ImageButton("##PlayState", (ImTextureID)icon, ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0.0f, 0.0f, 0.0f, 0.0f), tintColor) && toolbarEnabled)
			{
				if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate)
					OnScenePlay();
				else if (m_SceneState == SceneState::Play)
					OnSceneStop();
			}
		}

		if (hasSimulateButton)
		{
			if (hasPlayButton)
				ImGui::SameLine();

			auto icon = (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play) ? m_SimulateIconID : m_StopIconID;
			if (ImGui::ImageButton("##SimulateState", (ImTextureID)icon, ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0.0f, 0.0f, 0.0f, 0.0f), tintColor) && toolbarEnabled)
			{
				if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play)
					OnSceneSimulate();
				else if (m_SceneState == SceneState::Simulate)
					OnSceneStop();
			}
		}
		if (hasPauseButton)
		{
			bool isPaused = m_ActiveScene->IsPaused();
			ImGui::SameLine();
			{
				if (ImGui::ImageButton("##PauseState", (ImTextureID)m_PauseIconID, ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0.0f, 0.0f, 0.0f, 0.0f), tintColor) && toolbarEnabled)
				{
					m_ActiveScene->SetPaused(!isPaused);
				}
			}

			// Step button
			if (isPaused)
			{
				ImGui::SameLine();
				{
					bool isPaused = m_ActiveScene->IsPaused();
					if (ImGui::ImageButton("##StepState", (ImTextureID)m_StepIconID, ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0.0f, 0.0f, 0.0f, 0.0f), tintColor) && toolbarEnabled)
					{
						m_ActiveScene->StepFrames();
					}
				}
			}
		}

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);
		ImGui::End();
	}

	void EditorLayer::NewScene()
	{
		m_ActiveScene = std::make_shared<Scene>();
		m_SceneRenderer->SetActiveScene(m_ActiveScene);
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);

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
			m_ActiveScene = newScene;
			m_EditorScene = newScene;

			m_SceneRenderer->SetActiveScene(m_ActiveScene);
			m_SceneHierarchyPanel.SetContext(m_ActiveScene);
			m_EditorScenePath = path;
		}
	}

	void EditorLayer::SaveScene()
	{
		if (!m_EditorScenePath.empty())
			SerializeScene(m_ActiveScene, m_EditorScenePath);
		else
			SaveSceneAs();
	}

	void EditorLayer::SaveSceneAs()
	{
		std::string filepath = FileDialogs::SaveFile("VulkanEngine Scene (*.vkscene)\0*.vkscene\0");
		if (!filepath.empty())
		{
			SerializeScene(m_ActiveScene, filepath);
			m_EditorScenePath = filepath;
		}
	}

	void EditorLayer::OnScenePlay()
	{
		if (m_SceneState == SceneState::Simulate)
			OnSceneStop();

		m_SceneState = SceneState::Play;

		m_ActiveScene = Scene::CopyScene(m_EditorScene);
		m_ActiveScene->OnRuntimeStart();

		m_SceneRenderer->SetActiveScene(m_ActiveScene);
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
	}

	void EditorLayer::OnSceneSimulate()
	{
		if (m_SceneState == SceneState::Play)
			OnSceneStop();

		m_SceneState = SceneState::Simulate;

		m_ActiveScene = Scene::CopyScene(m_EditorScene);
		m_ActiveScene->OnSimulationStart();

		m_SceneRenderer->SetActiveScene(m_ActiveScene);
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
	}

	void EditorLayer::OnSceneStop()
	{
		VK_CORE_ASSERT(m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulate, "Invalid Scene State!");

		if (m_SceneState == SceneState::Play)
			m_ActiveScene->OnRuntimeStop();
		else if (m_SceneState == SceneState::Simulate)
			m_ActiveScene->OnSimulationStop();

		m_SceneState = SceneState::Edit;

		m_ActiveScene = m_EditorScene;

		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
	}

	void EditorLayer::OnScenePause()
	{
		if (m_SceneState == SceneState::Edit)
			return;

		m_ActiveScene->SetPaused(true);
	}

	void EditorLayer::SerializeScene(std::shared_ptr<Scene> scene, const std::filesystem::path& scenePath)
	{
		SceneSerializer serializer(scene);
		serializer.Serialize(scenePath.string());
	}

}