#pragma once
#include "Platform/Vulkan/VulkanDevice.h"
#include "Platform/Vulkan/VulkanGameObject.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "VulkanCore/Renderer/Camera.h"
#include "VulkanCore/Core/FrameInfo.h"

namespace VulkanCore {

	class PointLightSystem
	{
	public:
		PointLightSystem(VulkanDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~PointLightSystem();

		void Update(FrameInfo& frameInfo, UniformBufferDataComponent& ubo);
		void RenderLights(FrameInfo& frameInfo);

		inline VkPipelineLayout GetPipelineLayout() { return m_PipelineLayout; }
		inline VulkanPipeline* GetPipeline() const { return m_Pipeline.get(); }
	private:
		void CreatePipeline(VkRenderPass renderPass);
		void CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout);
	private:
		VulkanDevice& m_VulkanDevice;

		std::unique_ptr<VulkanPipeline> m_Pipeline;
		VkPipelineLayout m_PipelineLayout;
	};

}