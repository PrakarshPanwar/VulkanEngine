#include "vulkanpch.h"
#include "PointLightSystem.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"
#include "VulkanCore/Core/Components.h"

namespace VulkanCore {

	PointLightSystem::PointLightSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
	{
		CreatePipelineLayout(globalSetLayout);
		CreatePipeline(renderPass);
	}

	PointLightSystem::~PointLightSystem()
	{
		auto device = VulkanDevice::GetDevice();
		vkDestroyPipelineLayout(device->GetVulkanDevice(), m_PipelineLayout, nullptr);
	}

	void PointLightSystem::CreatePipeline(VkRenderPass renderPass)
	{
		auto device = VulkanDevice::GetDevice();

		PipelineConfigInfo pipelineConfig{};
		VulkanPipeline::DefaultPipelineConfigInfo(pipelineConfig);
		VulkanPipeline::EnableAlphaBlending(pipelineConfig);
		pipelineConfig.AttributeDescriptions.clear();
		pipelineConfig.BindingDescriptions.clear();
		pipelineConfig.RenderPass = renderPass;
		pipelineConfig.PipelineLayout = m_PipelineLayout;

		m_Pipeline = std::make_unique<VulkanPipeline>(
			*device, pipelineConfig,
			"assets/shaders/PointLightShader.vert",
			"assets/shaders/PointLightShader.frag"
		);
	}

	void PointLightSystem::CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout)
	{
		auto device = VulkanDevice::GetDevice();

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PointLightPushConstants);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		VK_CHECK_RESULT(vkCreatePipelineLayout(device->GetVulkanDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout), "Failed to Create Pipeline Layout!");
	}

}