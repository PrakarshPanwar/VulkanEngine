#pragma once
#include <entt.hpp>
#include "Core/glfw_vulkan.h"
#include "Core/VulkanPipeline.h"
#include "Core/Components.h"
#include "Core/VulkanDescriptor.h"

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
	private:
		entt::registry m_Registry;

		friend class Entity;
	};

}