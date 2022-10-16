#pragma once
#include "Platform/Vulkan/VulkanDevice.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "VulkanCore/Renderer/Camera.h"
#include "VulkanCore/Scene/Scene.h"

namespace VulkanCore {

	// TODO: I am working with this right now but in future,
	// these system will be shifted to struct SceneRendererData or in SceneRenderer itself
	class RenderSystem
	{
	public:
		RenderSystem(std::shared_ptr<VulkanRenderPass> renderPass, VkDescriptorSetLayout globalSetLayout);
		~RenderSystem();

		inline VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
		inline VulkanPipeline* GetPipeline() const { return m_Pipeline.get(); }
	private:
		void CreatePipeline(std::shared_ptr<VulkanRenderPass> renderPass);
		void CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout);
	private:
		std::unique_ptr<VulkanPipeline> m_Pipeline;
		VkPipelineLayout m_PipelineLayout;
	};

}