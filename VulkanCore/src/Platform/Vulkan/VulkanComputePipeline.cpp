#include "vulkanpch.h"
#include "VulkanComputePipeline.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Renderer.h"

namespace VulkanCore {

	namespace Utils {

		static VkShaderModule CreateShaderModule(const std::vector<uint32_t>& shaderSource)
		{
			auto device = VulkanContext::GetCurrentDevice();

			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = shaderSource.size() * sizeof(uint32_t);
			createInfo.pCode = shaderSource.data();

			VkShaderModule shaderModule;
			VK_CHECK_RESULT(vkCreateShaderModule(device->GetVulkanDevice(), &createInfo, nullptr, &shaderModule), "Failed to Create Shader Module!");

			return shaderModule;
		}

		static VkPipelineLayout CreatePipelineLayout(VulkanDescriptorSetLayout& descriptorLayout, size_t pushConstantSize = 0)
		{
			auto device = VulkanContext::GetCurrentDevice();

			VkPushConstantRange pushConstantRange{};
			pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			pushConstantRange.offset = 0;
			pushConstantRange.size = (uint32_t)pushConstantSize;

			std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ descriptorLayout.GetDescriptorSetLayout() };

			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
			pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
			pipelineLayoutInfo.pushConstantRangeCount = pushConstantSize == 0 ? 0 : 1;
			pipelineLayoutInfo.pPushConstantRanges = pushConstantSize == 0 ? nullptr : &pushConstantRange;

			VkPipelineLayout pipelineLayout;
			VK_CHECK_RESULT(vkCreatePipelineLayout(device->GetVulkanDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout), "Failed to Create Pipeline Layout!");
			return pipelineLayout;
		}

	}

	VulkanComputePipeline::VulkanComputePipeline(std::shared_ptr<Shader> shader)
		: m_Shader(shader)
	{
		CreateComputePipeline();
	}

	// TODO: Do this process in Render Thread
	VulkanComputePipeline::~VulkanComputePipeline()
	{
		auto device = VulkanContext::GetCurrentDevice();

		vkDestroyShaderModule(device->GetVulkanDevice(), m_compShaderModule, nullptr);

		if (m_PipelineLayout)
			vkDestroyPipelineLayout(device->GetVulkanDevice(), m_PipelineLayout, nullptr);

		vkDestroyPipeline(device->GetVulkanDevice(), m_ComputePipeline, nullptr);
	}

	void VulkanComputePipeline::Bind(VkCommandBuffer commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);
	}

	void VulkanComputePipeline::Dispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanComputePipeline::SetPushConstants(VkCommandBuffer cmdBuf, void* pcData, size_t size)
	{
		vkCmdPushConstants(cmdBuf,
			m_PipelineLayout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			0,
			(uint32_t)size,
			pcData);
	}

	void VulkanComputePipeline::CreateComputePipeline()
	{
		Renderer::Submit([this]()
		{
			auto device = VulkanContext::GetCurrentDevice();

			auto& shaderSources = m_Shader->GetShaderModules();

			m_compShaderModule = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::Compute]);

			m_DescriptorSetLayout = m_Shader->CreateDescriptorSetLayout();
			m_PipelineLayout = Utils::CreatePipelineLayout(*m_DescriptorSetLayout, m_Shader->GetPushConstantSize());

			VkPipelineShaderStageCreateInfo shaderStage;
			shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			shaderStage.module = m_compShaderModule;
			shaderStage.pName = "main";
			shaderStage.flags = 0;
			shaderStage.pNext = nullptr;
			shaderStage.pSpecializationInfo = nullptr;

			VkComputePipelineCreateInfo computePipelineInfo{};
			computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineInfo.stage = shaderStage;
			computePipelineInfo.layout = m_PipelineLayout;

			computePipelineInfo.basePipelineIndex = -1;
			computePipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

			VK_CHECK_RESULT(vkCreateComputePipelines(
				device->GetVulkanDevice(),
				VK_NULL_HANDLE,
				1,
				&computePipelineInfo,
				nullptr,
				&m_ComputePipeline),
				"Failed to Create Compute Pipeline!");
		});
	}

}
