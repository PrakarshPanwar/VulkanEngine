#include "vulkanpch.h"
#include "RenderSystem.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanMesh.h"

namespace VulkanCore {

	RenderSystem::RenderSystem(std::shared_ptr<VulkanRenderPass> renderPass, VkDescriptorSetLayout globalSetLayout)
	{
		PipelineSpecification spec;
		spec.pShader = Renderer::GetShader("FirstShader");
		spec.RenderPass = renderPass;
		spec.PushConstantSize = sizeof(PCModelData);
		spec.Layout = { Vertex::GetBindingDescriptions(), Vertex::GetAttributeDescriptions() };

		m_Pipeline = std::make_unique<VulkanPipeline>(spec);
	}

	RenderSystem::~RenderSystem()
	{
	}

	void RenderSystem::CreatePipeline(std::shared_ptr<VulkanRenderPass> renderPass)
	{
		auto device = VulkanContext::GetCurrentDevice();

		PipelineConfigInfo pipelineConfig{};
		pipelineConfig.RenderPass = renderPass;
		VulkanPipeline::SetDefaultPipelineConfiguration(pipelineConfig);
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
		pushConstantRange.size = sizeof(PCModelData);

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