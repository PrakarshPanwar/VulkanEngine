#pragma once
#include "VulkanContext.h"
#include "VulkanDescriptor.h"
#include "VulkanCore/Renderer/ComputePipeline.h"
#include "VulkanCore/Renderer/Shader.h"

namespace VulkanCore {

	class VulkanComputePipeline : public ComputePipeline
	{
	public:
		VulkanComputePipeline() = default;
		VulkanComputePipeline(std::shared_ptr<Shader> shader, const std::string& debugName = {});
		~VulkanComputePipeline();

		void Release();

		void Bind(VkCommandBuffer commandBuffer);
		void Dispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
		void Execute(VkCommandBuffer cmdBuf, VkDescriptorSet dstSet, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
		void SetPushConstants(VkCommandBuffer cmdBuf, void* pcData, size_t size, size_t offset = 0);
		void ReloadPipeline() override;

		inline std::shared_ptr<Shader> GetShader() const override { return m_Shader; }
		inline VkPipelineLayout GetVulkanPipelineLayout() const { return m_PipelineLayout; }
		inline std::shared_ptr<VulkanDescriptorSetLayout> GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
	private:
		void InvalidateComputePipeline();
	private:
		std::string m_DebugName;

		VkPipeline m_ComputePipeline;
		VkPipelineLayout m_PipelineLayout = nullptr;
		VkShaderModule m_ComputeShaderModule = nullptr;
		std::shared_ptr<Shader> m_Shader;
		std::shared_ptr<VulkanDescriptorSetLayout> m_DescriptorSetLayout;
	};

}
