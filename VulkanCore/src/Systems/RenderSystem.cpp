#include "vulkanpch.h"
#include "RenderSystem.h"

#include "Core/Assert.h"
#include "Core/Log.h"
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

	void RenderSystem::RenderGameObjects(FrameInfo& frameInfo)
	{
		m_Pipeline->Bind(frameInfo.CommandBuffer);

		vkCmdBindDescriptorSets(frameInfo.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
			0, 1, &frameInfo.GlobalDescriptorSet, 0, nullptr);

		for (auto& kv : frameInfo.GameObjects)
		{
			auto& object = kv.second;

			if (object.Model == nullptr)
				continue;

			PushConstantsDataComponent pushConstants{};
			pushConstants.ModelMatrix = object.Transform.GetTransform();
			pushConstants.NormalMatrix = object.Transform.GetNormalMatrix();
			pushConstants.timestep = (float)glfwGetTime();

			vkCmdPushConstants(frameInfo.CommandBuffer, m_PipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(PushConstantsDataComponent), &pushConstants);

			object.Model->Bind(frameInfo.CommandBuffer);
			object.Model->Draw(frameInfo.CommandBuffer);
		}
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