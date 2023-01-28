#include "vulkanpch.h"
#include "Scene.h"
#include "Entity.h"
#include "VulkanCore/Mesh/Mesh.h"
#include "Platform/Vulkan/VulkanDescriptor.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "SceneRenderer.h"
#include "VulkanCore/Renderer/VulkanRenderer.h"

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

		return entity;
	}

	void Scene::OnUpdateGeometry(const std::vector<VkCommandBuffer>& cmdBuffers, const std::shared_ptr<VulkanPipeline>& pipeline, const std::vector<VkDescriptorSet>& descriptorSet)
	{
		auto drawCmd = cmdBuffers[Renderer::GetCurrentFrameIndex()];
		auto dstSet = descriptorSet[Renderer::GetCurrentFrameIndex()];

		pipeline->Bind(drawCmd);

		vkCmdBindDescriptorSets(drawCmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline->GetVulkanPipelineLayout(),
			0, 1, &dstSet,
			0, nullptr);

		auto view = m_Registry.view<TransformComponent>();

		for (auto ent : view)
		{
			Entity entity = { ent, this };

			if (entity.HasComponent<MeshComponent>())
			{
				PCModelData pushConstants{};
				pushConstants.ModelMatrix = entity.GetComponent<TransformComponent>().GetTransform();
				pushConstants.NormalMatrix = entity.GetComponent<TransformComponent>().GetNormalMatrix();

				pipeline->SetPushConstants(drawCmd, &pushConstants, sizeof(PCModelData));
				Renderer::RenderMesh(entity.GetComponent<MeshComponent>().MeshInstance);
			}
		}
	}

	void Scene::OnUpdateGeometry(SceneRenderer* renderer)
	{
		auto view = m_Registry.view<TransformComponent>();

		for (auto ent : view)
		{
			Entity entity = { ent, this };

			if (entity.HasComponent<MeshComponent>())
			{
				auto& transformComponent = entity.GetComponent<TransformComponent>();
				renderer->SubmitMesh(entity.GetComponent<MeshComponent>().MeshInstance, transformComponent.GetTransform(), transformComponent.GetNormalMatrix());
			}
		}
	}

	// TODO: This should be managed by SceneRenderer
	void Scene::OnUpdateLights(const std::vector<VkCommandBuffer>& cmdBuffers, const std::shared_ptr<VulkanPipeline>& pipeline, const std::vector<VkDescriptorSet>& descriptorSet)
	{
		auto drawCmd = cmdBuffers[Renderer::GetCurrentFrameIndex()];
		auto dstSet = descriptorSet[Renderer::GetCurrentFrameIndex()];

		pipeline->Bind(drawCmd);

		vkCmdBindDescriptorSets(drawCmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline->GetVulkanPipelineLayout(),
			0, 1, &dstSet,
			0, nullptr);

		auto view = m_Registry.view<TransformComponent>();

		for (auto ent : view)
		{
			Entity entity = { ent, this };

			if (entity.HasComponent<PointLightComponent>())
			{
				Entity lightEntity = { ent, this };

				PCPointLight push{};
				auto& lightTransform = lightEntity.GetComponent<TransformComponent>();
				auto& pointLightComp = lightEntity.GetComponent<PointLightComponent>();

				push.Position = glm::vec4(lightTransform.Translation, 1.0f);
				push.Color = pointLightComp.PointLightInstance->Color;
				push.Radius = lightTransform.Scale.x;

				pipeline->SetPushConstants(drawCmd, &push, sizeof(PCPointLight));
				vkCmdDraw(drawCmd, 6, 1, 0, 0);
			}
		}
	}

	void Scene::UpdatePointLightUB(UBPointLights& ubo)
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