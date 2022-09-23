#pragma once
#include "Platform/Vulkan/VulkanDevice.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "VulkanCore/Renderer/Camera.h"

namespace VulkanCore {

	// TODO: I am working with this right now but in future,
	// these system will be shifted to struct SceneRendererData
	class PointLightSystem
	{
	public:
		PointLightSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~PointLightSystem();

		inline VkPipelineLayout GetPipelineLayout() { return m_PipelineLayout; }
		inline VulkanPipeline* GetPipeline() const { return m_Pipeline.get(); }
	private:
		void CreatePipeline(VkRenderPass renderPass);
		void CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout);
	private:
		std::unique_ptr<VulkanPipeline> m_Pipeline;
		VkPipelineLayout m_PipelineLayout;
	};

}