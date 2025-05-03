#include "vulkanpch.h"
#include "Scene.h"
#include "Entity.h"
#include "VulkanCore/Asset/AssetManager.h"
#include "VulkanCore/Asset/MaterialAsset.h"
#include "VulkanCore/Mesh/Mesh.h"
#include "VulkanCore/Core/UUID.h"
#include "SceneRenderer.h"

namespace VulkanCore {

	Scene::Scene()
	{
		m_PhysicsWorld = PhysicsWorld::Create();
	}

	Scene::~Scene()
	{
	}

	template<typename... Component>
	static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		([&]()
		{
			auto view = src.view<Component>();
			for (auto srcEntity : view)
			{
				entt::entity dstEntity = enttMap.at(src.get<IDComponent>(srcEntity).ID);

				auto& srcComponent = src.get<Component>(srcEntity);
				dst.emplace_or_replace<Component>(dstEntity, srcComponent);
			}
		}(), ...);
	}

	template<typename... Component>
	static void CopyComponent(ComponentGroup<Component...>, entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		CopyComponent<Component...>(dst, src, enttMap);
	}

	template<typename... Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		([&]()
		{
			if (src.HasComponent<Component>())
				dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
		}(), ...);
	}

	template<typename... Component>
	static void CopyComponentIfExists(ComponentGroup<Component...>, Entity dst, Entity src)
	{
		CopyComponentIfExists<Component...>(dst, src);
	}

	std::shared_ptr<Scene> Scene::CopyScene(std::shared_ptr<Scene> other)
	{
		std::shared_ptr<Scene> newScene = std::make_shared<Scene>();

		auto& srcSceneRegistry = other->m_Registry;
		auto& dstSceneRegistry = newScene->m_Registry;
		std::unordered_map<UUID, entt::entity> enttMap;

		// Create entities in new scene
		auto idView = srcSceneRegistry.view<IDComponent>();
		for (auto entityID : idView)
		{
			UUID uuid = srcSceneRegistry.get<IDComponent>(entityID).ID;
			const auto& name = srcSceneRegistry.get<TagComponent>(entityID).Tag;
			Entity newEntity = newScene->CreateEntityWithUUID(uuid, name);
			enttMap[uuid] = (entt::entity)newEntity;
		}

		// Copy components (except IDComponent and TagComponent)
		CopyComponent(AllComponents{}, dstSceneRegistry, srcSceneRegistry, enttMap);

		return newScene;
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = CreateEntityWithUUID(UUID{}, name);
		entity.AddComponent<TransformComponent>();

		return entity;
	}

	Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid);

		std::string_view tag = entity.AddComponent<TagComponent>(name).Tag;
		std::string_view entityNameView = tag.substr(0, tag.find_last_of('.'));
		std::string_view entityCount = tag.substr(tag.find_last_of('.') + 1);
		std::string entityName{ entityNameView };

		uint32_t count = 0;
		const auto charsResult = std::from_chars(entityCount.data(), entityCount.data() + entityCount.size(), count);
		if (charsResult.ec == std::errc())
		{
			if (m_TagDuplicateMap.contains(entityName))
				m_TagDuplicateMap[entityName] = std::max(count, m_TagDuplicateMap.at(entityName));
			else
				m_TagDuplicateMap[entityName] = count;
		}
		else if (!m_TagDuplicateMap.contains(entityName))
			m_TagDuplicateMap[entityName] = 0;

		return entity;
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		// Find existing entity name and duplication count for that particular name
		const auto& tag = entity.GetComponent<TagComponent>().Tag;
		std::string entityName = tag.substr(0, tag.find_last_of('.'));

		uint32_t count = ++m_TagDuplicateMap[entityName]; // Increment before assignment
		std::string name = entityName + '.' + std::to_string(count);

		Entity newEntity = CreateEntity(name);
		CopyComponentIfExists(AllComponents{}, newEntity, entity);
		return newEntity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

	void Scene::OnUpdateGeometry(SceneRenderer* renderer)
	{
		VK_CORE_PROFILE_FN("Submit-SubmitMeshes");
		auto view = m_Registry.view<TransformComponent, MeshComponent>();

		for (auto ent : view)
		{
			auto [transform, meshComponent] = view.get<TransformComponent, MeshComponent>(ent);

			std::shared_ptr<Mesh> mesh = AssetManager::GetAsset<Mesh>(meshComponent.MeshHandle);
			renderer->SubmitMesh(mesh, meshComponent.MaterialTableHandle, transform.GetTransform());
		}
	}

	void Scene::OnSelectGeometry(SceneRenderer* renderer)
	{
		auto view = m_Registry.view<TransformComponent, MeshComponent>();

		for (auto ent : view)
		{
			auto [transform, meshComponent] = view.get<TransformComponent, MeshComponent>(ent);

			std::shared_ptr<Mesh> mesh = AssetManager::GetAsset<Mesh>(meshComponent.MeshHandle);
			renderer->SubmitSelectedMesh(mesh, meshComponent.MaterialTableHandle, transform.GetTransform(), (uint32_t)ent);
		}
	}

	void Scene::DrawPhysicsBodies(std::shared_ptr<PhysicsDebugRenderer> debugRenderer)
	{
		m_PhysicsWorld->DrawPhysicsBodies(debugRenderer);
	}

	void Scene::OnUpdateLights(std::vector<glm::vec4>& pointLightPositions, std::vector<glm::vec4>& spotLightPositions, std::vector<uint32_t>& lightHandles)
	{
		{
			// Point Lights
			VK_CORE_PROFILE_FN("Scene-PointLights");
			auto view = m_Registry.view<TransformComponent, PointLightComponent>();

			for (auto ent : view)
			{
				auto [transform, pointLightComponent] = view.get<TransformComponent, PointLightComponent>(ent);

				glm::vec4 position = glm::vec4(transform.Translation, pointLightComponent.Radius);
				pointLightPositions.push_back(position);
				lightHandles.push_back((uint32_t)ent);
			}
		}

		{
			// Spot Lights
			VK_CORE_PROFILE_FN("Scene-SpotLights");
			auto view = m_Registry.view<TransformComponent, SpotLightComponent>();

			for (auto ent : view)
			{
				auto [transform, spotLightComponent] = view.get<TransformComponent, SpotLightComponent>(ent);

				glm::vec4 position = glm::vec4(transform.Translation, spotLightComponent.Radius);
				spotLightPositions.push_back(position);
				lightHandles.push_back((uint32_t)ent);
			}
		}
	}

	void Scene::UpdateLightsBuffer(UBPointLights& pointLights, UBSpotLights& spotLights, UBDirectionalLights& directionalLights)
	{
		// Point Lights
		{
			auto view = m_Registry.view<TransformComponent, PointLightComponent>();
			int lightIndex = 0;

			for (auto ent : view)
			{
				auto [transform, lightComponent] = view.get<TransformComponent, PointLightComponent>(ent);
				pointLights.PointLights[lightIndex++] =
				{
					glm::vec4(transform.Translation, 1.0f),
					lightComponent.Color,
					lightComponent.Radius,
					lightComponent.Falloff
				};
			}

			pointLights.LightCount = lightIndex;
		}

		// Spot Lights
		{
			auto view = m_Registry.view<TransformComponent, SpotLightComponent>();
			int lightIndex = 0;

			for (auto ent : view)
			{
				auto [transform, lightComponent] = view.get<TransformComponent, SpotLightComponent>(ent);
				glm::vec3 direction = glm::normalize(transform.Rotation);
				spotLights.SpotLights[lightIndex++] =
				{
					glm::vec4(transform.Translation, 1.0f),
					lightComponent.Color,
					direction,
					lightComponent.InnerCutoff,
					lightComponent.OuterCutoff,
					lightComponent.Radius,
					lightComponent.Falloff
				};
			}

			spotLights.LightCount = lightIndex;
		}

		// Directional Lights
		{
			auto view = m_Registry.view<DirectionalLightComponent>();
			int lightIndex = 0;

			for (auto ent : view)
			{
				auto lightComponent = view.get<DirectionalLightComponent>(ent);
				directionalLights.DirectionalLights[lightIndex++] =
				{
					lightComponent.Color,
					lightComponent.Direction,
					lightComponent.Falloff
				};
			}

			directionalLights.LightCount = lightIndex;
		}
	}

	DirectionalLightComponent Scene::GetDirectionalLightData(int index)
	{
		auto view = m_Registry.view<DirectionalLightComponent>();
		if (view.empty())
			return {};

		auto ent = *(view.begin() + index);
		return view.get<DirectionalLightComponent>(ent);
	}

	void Scene::OnRuntimeStart()
	{
		m_IsRunning = true;
		OnPhysicsWorldStart();
	}

	void Scene::OnRuntimeStop()
	{
		m_IsRunning = false;
		OnPhysicsWorldStop();
	}

	void Scene::OnSimulationStart()
	{
		OnPhysicsWorldStart();
	}

	void Scene::OnSimulationStop()
	{
		OnPhysicsWorldStop();
	}

	void Scene::OnUpdateRuntime(Timestep ts)
	{
		if (m_PhysicsWorld->IsValid() && !m_IsPaused || m_StepFrames-- > 0)
		{
			m_PhysicsWorld->Update(ts, this);
		}
	}

	void Scene::OnUpdateSimulation(Timestep ts)
	{
		if (m_PhysicsWorld->IsValid() && !m_IsPaused || m_StepFrames-- > 0)
		{
			m_PhysicsWorld->Update(ts, this);
		}
	}

	void Scene::StepFrames(int frames)
	{
		m_StepFrames = frames;
	}

	void Scene::OnPhysicsWorldStart()
	{
		m_PhysicsWorld->Init(this);
		m_PhysicsWorld->CreateBodies(this);
	}

	void Scene::OnPhysicsWorldStop()
	{
		m_PhysicsWorld->RemoveAndDestroyBodies(this);
		m_PhysicsWorld->DestroySystem();
	}

}
