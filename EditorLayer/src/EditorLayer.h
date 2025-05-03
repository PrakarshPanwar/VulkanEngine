#pragma once
#include "VulkanCore/Core/Layer.h"
#include "VulkanCore/Asset/AssetManagerBase.h"
#include "VulkanCore/Events/KeyEvent.h"
#include "VulkanCore/Events/MouseEvent.h"
#include "VulkanCore/Events/ApplicationEvent.h"
#include "VulkanCore/Scene/Scene.h"
#include "VulkanCore/Scene/SceneRenderer.h"
#include "VulkanCore/Renderer/EditorCamera.h"
#include "VulkanCore/Renderer/Texture.h"

#include <imgui.h>
#include <memory>
#include <filesystem>
#include <atomic>
#include <vector>

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"

namespace VulkanCore {

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		~EditorLayer();

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate() override;
		void OnEvent(Event& e) override;
		void OnImGuiRender() override;
	private:
		bool OnKeyEvent(KeyPressedEvent& e);
		bool OnMouseButtonEvent(MouseButtonPressedEvent& e);
		bool OnMouseScroll(MouseScrolledEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
		void RenderGizmo();
		void UI_Toolbar();

		void NewScene();
		void OpenScene();
		void OpenScene(const std::string& path);
		void SaveScene();
		void SaveSceneAs();
		void SerializeScene(std::shared_ptr<Scene> scene, const std::filesystem::path& scenePath);

		void OnScenePlay();
		void OnSceneSimulate();
		void OnSceneStop();
		void OnScenePause();
	private:
		enum class SceneState
		{
			Edit = 0, Play = 1, Simulate = 2
		} m_SceneState = SceneState::Edit;
	private:
		std::atomic<std::shared_ptr<Scene>> m_ActiveScene;
		std::shared_ptr<Scene> m_EditorScene;
		std::shared_ptr<SceneRenderer> m_SceneRenderer;
		std::filesystem::path m_EditorScenePath;
		EditorCamera m_EditorCamera;

		std::shared_ptr<AssetManagerBase> m_AssetManagerBase;

		bool m_ImGuiShowWindow = true, m_EnableSnap = false,
			m_ViewportHovered = false, m_ViewportFocused = false, m_WindowResized = false, 
			m_ShowApplicationStats = false, m_ShowCameraData = true, m_ShowPhysicsCollider = false;

		ImVec2 m_ViewportSize;
		glm::vec2 m_ViewportBounds[2];
		glm::vec4 m_TransformInput{};
		float m_TransformScalarInput = 0.0f;
		TransformComponent m_EntityTransform;
		int m_GizmoType = -1;
		float m_TranslationSnapValue = 0.5f, m_RotationSnapValue = 45.0f, m_ScaleSnapValue = 0.5f;
		uint8_t m_TransformInputMask = 0; // 0: X, 1: Y, 2: Z
		float m_LastFrameTime = 0.0f;

		SceneHierarchyPanel m_SceneHierarchyPanel;
		std::unique_ptr<ContentBrowserPanel> m_ContentBrowserPanel;

		std::shared_ptr<Texture2D> m_MenuIcon, m_PlayIcon, m_PauseIcon, m_StepIcon, m_SimulateIcon, m_StopIcon;
		VkDescriptorSet m_MenuIconID, m_PlayIconID, m_PauseIconID, m_StepIconID, m_SimulateIconID, m_StopIconID;
	};

}