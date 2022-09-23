#include "vulkanpch.h"
#include "RenderSystem.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

namespace VulkanCore {

	RenderSystem::RenderSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
	{
		CreatePipelineLayout(globalSetLayout);
		CreatePipeline(renderPass);
	}

	RenderSystem::~RenderSystem()
	{
		auto device = VulkanDevice::GetDevice();
		vkDestroyPipelineLayout(device->GetVulkanDevice(), m_PipelineLayout, nullptr);
	}

	void RenderSystem::CreatePipeline(VkRenderPass renderPass)
	{
		auto device = VulkanDevice::GetDevice();

		PipelineConfigInfo pipelineConfig{};
		VulkanPipeline::DefaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.RenderPass = renderPass;
		pipelineConfig.PipelineLayout = m_PipelineLayout;

#define USE_GEOMETRY_SHADER 0

#if USE_GEOMETRY_SHADER
		m_Pipeline = std::make_unique<VulkanPipeline>(
			m_VulkanDevice, pipelineConfig,
			"assets/shaders/FirstShader.vert",
			"assets/shaders/FirstShader.frag",
			"assets/shaders/FirstShader.geom"
		);
#else
		m_Pipeline = std::make_unique<VulkanPipeline>(
			*device, pipelineConfig,
			"assets/shaders/FirstShader.vert",
			"assets/shaders/FirstShader.frag"
		);
#endif
	}

	void RenderSystem::CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout) 
	{
		auto device = VulkanDevice::GetDevice();

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstantsDataComponent);

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