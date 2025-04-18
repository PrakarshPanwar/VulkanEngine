#pragma once
#include <entt.hpp>
#include "VulkanCore/Core/Components.h"
#include "VulkanCore/Renderer/EditorCamera.h"
#include "PhysicsWorld.h"

namespace VulkanCore {

	class Entity;
	class SceneRenderer;

	struct SceneEditorData
	{
		EditorCamera CameraData;
		glm::ivec2 ViewportMousePos;
		bool ViewportHovered, ShowPhysicsCollider;
	};

	class Scene : public Asset
	{
	public:
		Scene();
		~Scene();

		static std::shared_ptr<Scene> CopyScene(std::shared_ptr<Scene> other);

		Entity CreateEntity(const std::string& name);
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name);

		void OnUpdateGeometry(SceneRenderer* renderer);
		void OnSelectGeometry(SceneRenderer* renderer);
		void DrawPhysicsBodies(std::shared_ptr<PhysicsDebugRenderer> debugRenderer);
		void OnUpdateLights(std::vector<glm::vec4>& pointLightPositions, std::vector<glm::vec4>& spotLightPositions, std::vector<uint32_t>& lightHandles);
		void UpdateLightsBuffer(UBPointLights& pointLights, UBSpotLights& spotLights, UBDirectionalLights& directionalLights);
		DirectionalLightComponent GetDirectionalLightData(int index = 0);
		void DestroyEntity(Entity entity);

		void OnRuntimeStart();
		void OnRuntimeStop();

		void OnSimulationStart();
		void OnSimulationStop();

		void OnUpdateRuntime();
		void OnUpdateSimulation();
		//void OnUpdateEditor();

		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

		bool IsRunning() const { return m_IsRunning; }
		bool IsPaused() const { return m_IsPaused; }

		void SetPaused(bool paused) { m_IsPaused = paused; }
		void StepFrames(int frames = 1);

		inline AssetType GetType() const override { return AssetType::Scene; }
		static AssetType GetStaticType() { return AssetType::Scene; }
	private:
		void OnPhysicsWorldStart();
		void OnPhysicsWorldStop();
	private:
		entt::registry m_Registry;
		bool m_IsRunning = false;
		bool m_IsPaused = false;
		int m_StepFrames = 0;

		std::unique_ptr<PhysicsWorld> m_PhysicsWorld;

		friend class Entity;
		friend class SceneHierarchyPanel;
		friend class SceneSerializer;
		friend class SceneRenderer;
	};

}