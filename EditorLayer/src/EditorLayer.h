#pragma once
#include "VulkanCore/Core/Layer.h"
#include "VulkanCore/Asset/AssetManagerBase.h"
#include "VulkanCore/Events/KeyEvent.h"
#include "VulkanCore/Events/MouseEvent.h"
#include "VulkanCore/Scene/Scene.h"
#include "VulkanCore/Scene/SceneRenderer.h"
#include "VulkanCore/Renderer/EditorCamera.h"
#include "VulkanCore/Renderer/Texture.h"

#include <imgui.h>
#include <memory>
#include <filesystem>
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
		bool OnKeyEvent(KeyPressedEvent& keyEvent);
		bool OnMouseScroll(MouseScrolledEvent& mouseScroll);
		bool OnWindowResize(WindowResizeEvent& windowEvent);
		void RenderGizmo();

		void NewScene();
		void OpenScene();
		void OpenScene(const std::string& path);
		void SaveScene();
		void SaveSceneAs();
		void SerializeScene(std::shared_ptr<Scene> scene, const std::filesystem::path& scenePath);
	private:
		std::shared_ptr<Scene> m_Scene;
		std::shared_ptr<SceneRenderer> m_SceneRenderer;
		std::filesystem::path m_EditorScenePath;
		EditorCamera m_EditorCamera;

		std::shared_ptr<AssetManagerBase> m_AssetManagerBase;

		bool m_ImGuiShowWindow = true, m_ViewportHovered = false,
			m_ViewportFocused = false, m_WindowResized = false,
			m_EnableSnap = false;

		ImVec2 m_ViewportSize = { 1904.0f, 991.0f }; // TODO: Calculate this by function
		glm::vec2 m_ViewportBounds[2];
		int m_GizmoType = -1;
		float m_TranslationSnapValue = 0.5f, m_RotationSnapValue = 45.0f, m_ScaleSnapValue = 0.5f;

		SceneHierarchyPanel m_SceneHierarchyPanel;
		std::unique_ptr<ContentBrowserPanel> m_ContentBrowserPanel;

		std::shared_ptr<Texture2D> m_MenuIcon;
		VkDescriptorSet m_MenuIconID;
	};

}