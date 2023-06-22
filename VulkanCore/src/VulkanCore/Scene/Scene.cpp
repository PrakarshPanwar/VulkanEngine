#include "vulkanpch.h"
#include "Scene.h"
#include "Entity.h"
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
		return CreateEntityWithUUID(UUID{}, name);
	}

	Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid);
		entity.AddComponent<TagComponent>(name);
		entity.AddComponent<TransformComponent>();

		return entity;
	}

	void Scene::OnUpdateGeometry(SceneRenderer* renderer)
	{
		VK_CORE_PROFILE_FN("Submit-SubmitMeshes");
		auto view = m_Registry.view<TransformComponent, MeshComponent>();

		for (auto ent : view)
		{
			auto [transform, meshComponent] = view.get<TransformComponent, MeshComponent>(ent);
			renderer->SubmitMesh(meshComponent.MeshInstance, meshComponent.MaterialTable, transform.GetTransform());
		}
	}

	void Scene::OnUpdateLights(std::vector<glm::vec4>& pointLightPositions, std::vector<glm::vec4>& spotLightPositions)
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
			}
		}
	}

	void Scene::UpdatePointLightUB(UBPointLights& ubo)
	{
		auto view = m_Registry.view<TransformComponent, PointLightComponent>();

		int lightIndex = 0;
		for (auto ent : view)
		{
			auto [transform, lightComponent] = view.get<TransformComponent, PointLightComponent>(ent);
			ubo.PointLights[lightIndex++] =
			{
				glm::vec4(transform.Translation, 1.0f),
				lightComponent.Color,
				lightComponent.Radius,
				lightComponent.Falloff
			};
		}

		ubo.LightCount = lightIndex;
	}

	void Scene::UpdateSpotLightUB(UBSpotLights& ubo)
	{
		auto view = m_Registry.view<TransformComponent, SpotLightComponent>();
		int lightIndex = 0;

		for (auto ent : view)
		{
			auto [transform, lightComponent] = view.get<TransformComponent, SpotLightComponent>(ent);
			glm::vec3 direction = -glm::normalize(glm::mat3(transform.GetTransform()) * glm::vec3(1.0f));
			ubo.SpotLights[lightIndex++] =
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

		ubo.LightCount = lightIndex;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

}