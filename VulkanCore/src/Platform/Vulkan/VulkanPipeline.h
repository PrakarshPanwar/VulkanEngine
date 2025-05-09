#pragma once
#include "VulkanDevice.h"
#include "VulkanDescriptor.h"
#include "VulkanCore/Renderer/Pipeline.h"
#include "VulkanCore/Renderer/Buffer.h"

namespace VulkanCore {

	struct PipelineConfiguration
	{
		PipelineConfiguration() = default;

		VkPipelineViewportStateCreateInfo ViewportInfo{};
		VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo{};
		VkPipelineTessellationStateCreateInfo TessellationInfo{};
		VkPipelineRasterizationStateCreateInfo RasterizationInfo{};
		VkPipelineMultisampleStateCreateInfo MultisampleInfo{};
		std::vector<VkPipelineColorBlendAttachmentState> ColorBlendAttachments;
		VkPipelineColorBlendStateCreateInfo ColorBlendInfo{};
		VkPipelineDepthStencilStateCreateInfo DepthStencilInfo{};
		std::vector<VkDynamicState> DynamicStateEnables;
		VkPipelineDynamicStateCreateInfo DynamicStateInfo{};
		uint32_t Subpass = 0;
	};

	class VulkanPipeline : public Pipeline
	{
	public:
		VulkanPipeline() = default;
		VulkanPipeline(const PipelineSpecification& spec);
		~VulkanPipeline();

		void Bind(VkCommandBuffer commandBuffer);
		void SetPushConstants(VkCommandBuffer cmdBuf, void* pcData, size_t size);
		void ReloadPipeline() override;

		VkPipelineLayout GetVulkanPipelineLayout() const { return m_PipelineLayout; }
		PipelineSpecification GetSpecification() const override { return m_Specification; }
		std::shared_ptr<VulkanDescriptorSetLayout> GetDescriptorSetLayout(uint32_t index = 0) const { return m_DescriptorSetLayout[index]; }
	private:
		void InvalidateGraphicsPipeline();
		void CreatePipelineCache();
		void Release();
	private:
		PipelineSpecification m_Specification;

		VkPipeline m_GraphicsPipeline = nullptr;
		VkPipelineLayout m_PipelineLayout = nullptr;
		std::map<ShaderType, VkShaderModule> m_ShaderModules;

		std::shared_ptr<Shader> m_Shader;
		std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> m_DescriptorSetLayout;
	};

}
