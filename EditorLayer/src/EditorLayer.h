#pragma once
#include "VulkanCore/Core/Layer.h"
#include "VulkanCore/Events/KeyEvent.h"
#include "VulkanCore/Events/MouseEvent.h"
#include "VulkanCore/Scene/Scene.h"
#include "VulkanCore/Scene/SceneRenderer.h"
#include "VulkanCore/Renderer/EditorCamera.h"

#include "Platform/Vulkan/VulkanSwapChain.h"
#include "Platform/Vulkan/VulkanBuffer.h"
#include "Platform/Vulkan/VulkanTexture.h"

#include "Panels/SceneHierarchyPanel.h"

#include <imgui.h>

#include <filesystem>

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
		void UpdateSceneDescriptors();
		void LoadEntities();
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

		std::vector<VkDescriptorSet> m_SceneImages;
		bool m_ImGuiShowWindow = true, m_ViewportHovered = false, m_ViewportFocused = false, m_WindowResized = false;
		ImVec2 m_ViewportSize = { 1904.0f, 991.0f }; // TODO: Calculate this by function

		glm::vec2 m_ViewportBounds[2];
		int m_GizmoType = -1;

		SceneHierarchyPanel m_SceneHierarchyPanel;

		std::shared_ptr<VulkanTexture> m_MenuIcon;
		VkDescriptorSet m_MenuIconID;
	};

}