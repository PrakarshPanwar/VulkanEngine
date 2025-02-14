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
	}

	Scene::~Scene()
	{
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
		entity.AddComponent<TagComponent>(name);

		return entity;
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

		return view.get<DirectionalLightComponent>(view[index]);
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

}