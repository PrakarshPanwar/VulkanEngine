#include "vulkanpch.h"
#include "PointLightSystem.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"
#include "VulkanCore/Core/Components.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "VulkanCore/Renderer/Renderer.h"

namespace VulkanCore {

	PointLightSystem::PointLightSystem(std::shared_ptr<VulkanRenderPass> renderPass, VkDescriptorSetLayout globalSetLayout)
	{
#if USE_PIPELINE_SPEC
		PipelineSpecification spec;
		spec.pShader = Renderer::GetShader("PointLightShader");
		spec.Blend = true;
		spec.RenderPass = renderPass;
		spec.PushConstantSize = sizeof(PCPointLight);

		m_Pipeline = std::make_unique<VulkanPipeline>(spec);
#else
		CreatePipelineLayout(globalSetLayout);
		CreatePipeline(renderPass);
#endif
	}

	PointLightSystem::~PointLightSystem()
	{
#if !USE_PIPELINE_SPEC
		auto device = VulkanContext::GetCurrentDevice();
		vkDestroyPipelineLayout(device->GetVulkanDevice(), m_PipelineLayout, nullptr);
#endif
	}

	void PointLightSystem::CreatePipeline(std::shared_ptr<VulkanRenderPass> renderPass)
	{
		auto device = VulkanContext::GetCurrentDevice();

		PipelineConfigInfo pipelineConfig{};
		pipelineConfig.RenderPass = renderPass;
		VulkanPipeline::SetDefaultPipelineConfiguration(pipelineConfig);
		VulkanPipeline::EnableAlphaBlending(pipelineConfig);
		pipelineConfig.AttributeDescriptions.clear();
		pipelineConfig.BindingDescriptions.clear();
		pipelineConfig.PipelineLayout = m_PipelineLayout;

		m_Pipeline = std::make_unique<VulkanPipeline>(
			pipelineConfig,
			"assets/shaders/PointLightShader.vert",
			"assets/shaders/PointLightShader.frag"
		);
	}

	void PointLightSystem::CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout)
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PCPointLight);

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