#pragma once
#include "VulkanContext.h"
#include "VulkanCore/Core/Shader.h"

namespace VulkanCore {

	class VulkanComputePipeline
	{
	public:
		VulkanComputePipeline() = default;
		VulkanComputePipeline(std::shared_ptr<Shader> shader);

		~VulkanComputePipeline();

		void Bind(VkCommandBuffer commandBuffer);
		void Dispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
	private:
		void CreateComputePipeline();
	private:
		VkPipeline m_ComputePipeline;
		VkPipelineLayout m_PipelineLayout = nullptr;
		VkShaderModule m_compShaderModule = nullptr;
		std::shared_ptr<Shader> m_Shader;
		std::shared_ptr<VulkanDescriptorSetLayout> m_DescriptorSetLayout;
	};

}
