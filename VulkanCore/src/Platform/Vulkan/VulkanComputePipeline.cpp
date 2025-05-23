#include "vulkanpch.h"
#include "VulkanComputePipeline.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "VulkanShader.h"

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

			std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ descriptorLayout.GetVulkanDescriptorSetLayout() };

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

	VulkanComputePipeline::VulkanComputePipeline(std::shared_ptr<Shader> shader, const std::string& debugName)
		: m_DebugName(debugName), m_Shader(shader)
	{
		Renderer::Submit([this]
		{
			InvalidateComputePipeline();
		});
	}

	VulkanComputePipeline::~VulkanComputePipeline()
	{
		Release();
	}

	void VulkanComputePipeline::Release()
	{
		Renderer::SubmitResourceFree([pipeline = m_ComputePipeline, layout = m_PipelineLayout, computeShaderModule = m_ComputeShaderModule]
		{
			auto device = VulkanContext::GetCurrentDevice();

			vkDestroyShaderModule(device->GetVulkanDevice(), computeShaderModule, nullptr);

			if (layout)
				vkDestroyPipelineLayout(device->GetVulkanDevice(), layout, nullptr);

			vkDestroyPipeline(device->GetVulkanDevice(), pipeline, nullptr);
		});

		m_ComputePipeline = nullptr;
	}

	void VulkanComputePipeline::Bind(VkCommandBuffer commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);
	}

	void VulkanComputePipeline::Dispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanComputePipeline::Execute(VkCommandBuffer cmdBuf, VkDescriptorSet dstSet, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		vkCmdBindDescriptorSets(cmdBuf,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			m_PipelineLayout, 0, 1,
			&dstSet, 0, nullptr);

		vkCmdDispatch(cmdBuf, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanComputePipeline::SetPushConstants(VkCommandBuffer cmdBuf, void* pcData, size_t size, size_t offset)
	{
		vkCmdPushConstants(cmdBuf,
			m_PipelineLayout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			(uint32_t)offset,
			(uint32_t)size,
			pcData);
	}

	void VulkanComputePipeline::ReloadPipeline()
	{
		if (m_Shader->GetReloadFlag())
		{
			Release();
			InvalidateComputePipeline();
			m_Shader->ResetReloadFlag();
		}
	}

	void VulkanComputePipeline::InvalidateComputePipeline()
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto shader = std::static_pointer_cast<VulkanShader>(m_Shader);

		auto& shaderSources = shader->GetShaderModules();

		m_ComputeShaderModule = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::Compute]);

		m_DescriptorSetLayout = shader->CreateDescriptorSetLayout();
		m_PipelineLayout = Utils::CreatePipelineLayout(*m_DescriptorSetLayout, shader->GetPushConstantSize());

		VkPipelineShaderStageCreateInfo shaderStage;
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStage.module = m_ComputeShaderModule;
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

		VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(), VK_OBJECT_TYPE_PIPELINE, m_DebugName, m_ComputePipeline);
	}

}
