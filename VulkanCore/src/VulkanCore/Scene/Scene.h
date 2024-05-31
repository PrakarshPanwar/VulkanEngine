#pragma once
#include <entt.hpp>
#include "VulkanCore/Core/Components.h"
#include "VulkanCore/Renderer/EditorCamera.h"

namespace VulkanCore {

	class Entity;
	class SceneRenderer;

	struct SceneEditorData
	{
		EditorCamera CameraData;
		glm::ivec2 ViewportMousePos;
		bool ViewportHovered;
	};

	class Scene : public Asset
	{
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name);
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name);

		void OnUpdateGeometry(SceneRenderer* renderer);
		void OnSelectGeometry(SceneRenderer* renderer);
		void OnUpdateLights(std::vector<glm::vec4>& pointLightPositions, std::vector<glm::vec4>& spotLightPositions, std::vector<uint32_t>& lightHandles);
		void UpdateLightsBuffer(UBPointLights& pointLights, UBSpotLights& spotLights, UBDirectionalLights& directionalLights);
		DirectionalLightComponent GetDirectionalLightData(int index = 0);
		void DestroyEntity(Entity entity);

		inline AssetType GetType() const override { return AssetType::Scene; }
		static AssetType GetStaticType() { return AssetType::Scene; }
	private:
		entt::registry m_Registry;

		friend class Entity;
		friend class SceneHierarchyPanel;
		friend class SceneSerializer;
		friend class SceneRenderer;
	};

}