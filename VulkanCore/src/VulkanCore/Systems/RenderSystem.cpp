#include "vulkanpch.h"
#include "RenderSystem.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"
#include <glm/gtc/constants.hpp>

namespace VulkanCore {

	RenderSystem::RenderSystem(VulkanDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
		: m_VulkanDevice(device)
	{
		CreatePipelineLayout(globalSetLayout);
		CreatePipeline(renderPass);
	}

	RenderSystem::~RenderSystem()
	{
		vkDestroyPipelineLayout(m_VulkanDevice.GetVulkanDevice(), m_PipelineLayout, nullptr);
	}

	void RenderSystem::CreatePipeline(VkRenderPass renderPass)
	{
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
			m_VulkanDevice, pipelineConfig,
			"assets/shaders/FirstShader.vert",
			"assets/shaders/FirstShader.frag"
		);
#endif
	}

	void RenderSystem::CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout)
{
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

		VK_CHECK_RESULT(vkCreatePipelineLayout(m_VulkanDevice.GetVulkanDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout), "Failed to Create Pipeline Layout!");
	}

}