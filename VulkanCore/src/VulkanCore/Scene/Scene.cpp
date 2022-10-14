#include "vulkanpch.h"
#include "Scene.h"
#include "Entity.h"
#include "Platform/Vulkan/VulkanMesh.h"
#include "Platform/Vulkan/VulkanDescriptor.h"

namespace VulkanCore {

	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<TagComponent>(name);
		//entity.AddComponent<TransformComponent>();

		return entity;
	}

	void Scene::OnUpdate(SceneInfo& sceneInfo)
	{
		sceneInfo.ScenePipeline->Bind(sceneInfo.CommandBuffer);

		vkCmdBindDescriptorSets(sceneInfo.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, sceneInfo.PipelineLayout,
			0, 1, &sceneInfo.SceneDescriptorSet, 0, nullptr);

		auto view = m_Registry.view<TransformComponent>();

		for (auto ent : view)
		{
			Entity entity = { ent, this };

			PushConstantsDataComponent pushConstants{};
			pushConstants.ModelMatrix = entity.GetComponent<TransformComponent>().GetTransform();
			pushConstants.NormalMatrix = entity.GetComponent<TransformComponent>().GetNormalMatrix();
			pushConstants.timestep = (float)glfwGetTime();

			vkCmdPushConstants(sceneInfo.CommandBuffer, sceneInfo.PipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(PushConstantsDataComponent), &pushConstants);

			if (entity.HasComponent<MeshComponent>())
			{
				entity.GetComponent<MeshComponent>().Mesh->Bind(sceneInfo.CommandBuffer);
				entity.GetComponent<MeshComponent>().Mesh->Draw(sceneInfo.CommandBuffer);
			}
		}
	}

	void Scene::OnUpdateLights(SceneInfo& sceneInfo)
	{
		sceneInfo.ScenePipeline->Bind(sceneInfo.CommandBuffer);

		vkCmdBindDescriptorSets(sceneInfo.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, sceneInfo.PipelineLayout,
			0, 1, &sceneInfo.SceneDescriptorSet, 0, nullptr);

		auto view = m_Registry.view<TransformComponent>();

		for (auto ent : view)
		{
			Entity lightEntity = { ent, this };

			PointLightPushConstants push{};
			auto& lightTransform = lightEntity.GetComponent<TransformComponent>();

			if (lightEntity.HasComponent<PointLightComponent>())
			{
				auto& pointLightComp = lightEntity.GetComponent<PointLightComponent>();

				push.Position = glm::vec4(lightTransform.Translation, 1.0f);
				push.Color = pointLightComp.PointLightInstance->Color;
				push.Radius = lightTransform.Scale.x;

				vkCmdPushConstants(sceneInfo.CommandBuffer, sceneInfo.PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					0, sizeof(PointLightPushConstants), &push);

				vkCmdDraw(sceneInfo.CommandBuffer, 6, 1, 0, 0);
			}
		}
	}

	void Scene::UpdateUniformBuffer(UBCameraandLights& ubo)
	{
		auto view = m_Registry.view<TransformComponent>();
		int lightIndex = 0;

		for (auto ent : view)
		{
			Entity lightEntity = { ent, this };

			auto& lightTransform = lightEntity.GetComponent<TransformComponent>();

			if (lightEntity.HasComponent<PointLightComponent>())
			{
				auto& pointLightComp = lightEntity.GetComponent<PointLightComponent>();

				ubo.PointLights[lightIndex].Position = glm::vec4(lightTransform.Translation, 1.0f);
				ubo.PointLights[lightIndex].Color = pointLightComp.PointLightInstance->Color;

				lightIndex++;
			}
		}

		ubo.NumLights = lightIndex;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

}