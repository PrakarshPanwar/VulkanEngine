#pragma once
#include "VulkanCore/Scene/Entity.h"
#include "VulkanCore/Scene/Scene.h"
#include <memory>

namespace VulkanCore {

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(std::shared_ptr<Scene> sceneContext);

		~SceneHierarchyPanel();

		void OnImGuiRender();

		Entity GetSelectedEntity() const { return m_SelectionContext; }
		void SetSelectedEntity(Entity entity);
	private:
		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);
	private:
		std::shared_ptr<Scene> m_Context;
		Entity m_SelectionContext;
	};

}