#include "vulkanpch.h"
#include "PointLightSystem.h"

#include "Core/Assert.h"
#include "Core/Log.h"

namespace VulkanCore {

	PointLightSystem::PointLightSystem(VulkanDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
		: m_VulkanDevice(device)
	{
		CreatePipelineLayout(globalSetLayout);
		CreatePipeline(renderPass);
	}

	PointLightSystem::~PointLightSystem()
	{
		vkDestroyPipelineLayout(m_VulkanDevice.GetVulkanDevice(), m_PipelineLayout, nullptr);
	}

	void PointLightSystem::Update(FrameInfo& frameInfo, UniformBufferDataComponent& ubo)
	{
		int lightIndex = 0;
		for (auto& GameObject : frameInfo.GameObjects)
		{
			auto& obj = GameObject.second;

			if (obj.PointLight == nullptr)
				continue;

			ubo.PointLights[lightIndex].Position = glm::vec4(obj.Transform.Translation, 1.0f);
			ubo.PointLights[lightIndex].Color = glm::vec4(obj.Color, obj.PointLight->LightIntensity);
			lightIndex++;
		}

		ubo.NumLights = lightIndex;
	}

	void PointLightSystem::RenderLights(FrameInfo& frameInfo)
	{
		m_Pipeline->Bind(frameInfo.CommandBuffer);

		vkCmdBindDescriptorSets(frameInfo.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
			0, 1, &frameInfo.GlobalDescriptorSet, 0, nullptr);

		for (auto& pointLight : frameInfo.GameObjects)
		{
			auto& obj = pointLight.second;
			if (obj.PointLight == nullptr)
				continue;

			PointLightPushConstants push{};
			push.Position = glm::vec4(obj.Transform.Translation, 1.0f);
			push.Color = glm::vec4(obj.Color, obj.PointLight->LightIntensity);
			push.Radius = obj.Transform.Scale.x;

			vkCmdPushConstants(frameInfo.CommandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(PointLightPushConstants), &push);

			vkCmdDraw(frameInfo.CommandBuffer, 6, 1, 0, 0);
		}

	}

	void PointLightSystem::CreatePipeline(VkRenderPass renderPass)
	{
		PipelineConfigInfo pipelineConfig{};
		VulkanPipeline::DefaultPipelineConfigInfo(pipelineConfig);
		VulkanPipeline::EnableAlphaBlending(pipelineConfig);
		pipelineConfig.AttributeDescriptions.clear();
		pipelineConfig.BindingDescriptions.clear();
		pipelineConfig.RenderPass = renderPass;
		pipelineConfig.PipelineLayout = m_PipelineLayout;

		m_Pipeline = std::make_unique<VulkanPipeline>(
			m_VulkanDevice, pipelineConfig,
			"assets/shaders/PointLightShader.vert",
			"assets/shaders/PointLightShader.frag"
		);
	}

	void PointLightSystem::CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout)
	{
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

		VK_CHECK_RESULT(vkCreatePipelineLayout(m_VulkanDevice.GetVulkanDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout), "Failed to Create Pipeline Layout!");
	}

}