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
		VkDescriptorSet SceneDescriptorSet;
	};

	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name);
		void OnUpdate(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VulkanPipeline* pipeline, VkDescriptorSet descriptorSet);
		void OnUpdate(SceneInfo& sceneInfo);
		void OnUpdateLights(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VulkanPipeline* pipeline, VkDescriptorSet descriptorSet);
		void OnUpdateLights(SceneInfo& sceneInfo);
		void UpdateUniformBuffer(UniformBufferDataComponent& ubo);
		void DestroyEntity(Entity entity);
	private:
		entt::registry m_Registry;

		friend class Entity;
		friend class SceneHierarchyPanel;
	};

}