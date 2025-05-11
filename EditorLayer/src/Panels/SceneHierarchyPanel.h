#pragma once
#include "VulkanCore/Scene/Entity.h"
#include "VulkanCore/Scene/Scene.h"

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
		void SetContext(std::shared_ptr<Scene> context);
	private:
		template<typename T> requires IsComponentType<T>
		void DisplayAddComponentEntry(const std::string& entryName);

		template<>
		void DisplayAddComponentEntry<SkyLightComponent>(const std::string& entryName);

		template<>
		void DisplayAddComponentEntry<DirectionalLightComponent>(const std::string& entryName);

		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);
	private:
		std::shared_ptr<Scene> m_Context;
		Entity m_SelectionContext;
	};

}
