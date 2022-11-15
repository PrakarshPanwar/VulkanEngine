#pragma once
#include <entt.hpp>
#include "VulkanCore/Core/glfw_vulkan.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "VulkanCore/Core/Components.h"
#include "Platform/Vulkan/VulkanDescriptor.h"

namespace VulkanCore {

	class Entity;

	struct SceneInfo
	{
		VkCommandBuffer CommandBuffer;
		VulkanPipeline* ScenePipeline;
		VkPipelineLayout PipelineLayout;
		VkDescriptorSet DescriptorSet;
	};

	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name);
		void OnUpdate(SceneInfo& sceneInfo);
		void OnUpdateLights(SceneInfo& sceneInfo);
		void OnUpdateGeometry(const std::vector<VkCommandBuffer>& cmdBuffers, const std::shared_ptr<VulkanPipeline>& pipeline, const std::vector<VkDescriptorSet>& descriptorSet);
		void OnUpdateLights(const std::vector<VkCommandBuffer>& cmdBuffers, const std::shared_ptr<VulkanPipeline>& pipeline, const std::vector<VkDescriptorSet>& descriptorSet);
		void UpdatePointLightUB(UBPointLights& ubo);
		void DestroyEntity(Entity entity);
	private:
		entt::registry m_Registry;

		friend class Entity;
		friend class SceneHierarchyPanel;
		friend class SceneRenderer;
	};

}