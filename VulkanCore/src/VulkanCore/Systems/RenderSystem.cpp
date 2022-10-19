#include "vulkanpch.h"
#include "RenderSystem.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"
#include "Platform/Vulkan/VulkanContext.h"

namespace VulkanCore {

	RenderSystem::RenderSystem(std::shared_ptr<VulkanRenderPass> renderPass, VkDescriptorSetLayout globalSetLayout)
	{
		CreatePipelineLayout(globalSetLayout);
		CreatePipeline(renderPass);
	}

	RenderSystem::~RenderSystem()
	{
		auto device = VulkanContext::GetCurrentDevice();
		vkDestroyPipelineLayout(device->GetVulkanDevice(), m_PipelineLayout, nullptr);
	}

	void RenderSystem::CreatePipeline(std::shared_ptr<VulkanRenderPass> renderPass)
	{
		auto device = VulkanContext::GetCurrentDevice();

		PipelineConfigInfo pipelineConfig{};
		pipelineConfig.RenderPass = renderPass;
		VulkanPipeline::DefaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.PipelineLayout = m_PipelineLayout;

#define USE_GEOMETRY_SHADER 0

#if USE_GEOMETRY_SHADER
		m_Pipeline = std::make_unique<VulkanPipeline>(
			pipelineConfig,
			"assets/shaders/FirstShader.vert",
			"assets/shaders/FirstShader.frag",
			"assets/shaders/FirstShader.geom"
		);
#else
		m_Pipeline = std::make_unique<VulkanPipeline>(
			pipelineConfig,
			"assets/shaders/FirstShader.vert",
			"assets/shaders/FirstShader.frag"
		);
#endif
	}

	void RenderSystem::CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout) 
	{
		auto device = VulkanContext::GetCurrentDevice();

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