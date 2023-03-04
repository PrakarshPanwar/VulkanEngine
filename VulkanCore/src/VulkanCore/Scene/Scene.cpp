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
		entity.AddComponent<TransformComponent>();

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
		auto view = m_Registry.view<TransformComponent, MeshComponent>();

		for (auto ent : view)
		{
			auto [transform, meshComponent] = view.get<TransformComponent, MeshComponent>(ent);
			renderer->SubmitMesh(meshComponent.MeshInstance, meshComponent.MeshInstance->GetMeshSource()->GetMaterial(), transform.GetTransform());
		}
	}

	// TODO: This should be managed by SceneRenderer
	void Scene::OnUpdateLights(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, const std::shared_ptr<VulkanPipeline>& pipeline, const std::vector<VkDescriptorSet>& descriptorSet)
	{
		auto drawCmd = cmdBuffer->GetActiveCommandBuffer();
		auto dstSet = descriptorSet[Renderer::GetCurrentFrameIndex()];

		Renderer::Submit([drawCmd, pipeline, dstSet]
		{
			pipeline->Bind(drawCmd);

			vkCmdBindDescriptorSets(drawCmd,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipeline->GetVulkanPipelineLayout(),
				0, 1, &dstSet,
				0, nullptr);
		});

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

				push.Position = glm::vec4(lightTransform.Translation, pointLightComp.Radius);
				push.Color = pointLightComp.Color;

				Renderer::Submit([drawCmd, pipeline, push]
				{
					pipeline->SetPushConstants(drawCmd, (void*)&push, sizeof(PCPointLight));
					vkCmdDraw(drawCmd, 6, 1, 0, 0);
				});
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