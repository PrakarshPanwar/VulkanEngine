#pragma once
#include "VulkanDevice.h"
#include "VulkanCore/Core/Shader.h"
#include "VulkanRenderPass.h"

namespace VulkanCore {

	struct PipelineSpecification
	{
		std::shared_ptr<Shader> pShader;
		std::shared_ptr<VulkanRenderPass> RenderPass;
		bool BackfaceCulling = false;
		bool DepthTest = true;
		bool DepthWrite = true;
		std::pair<std::vector<VkVertexInputBindingDescription>,
			std::vector<VkVertexInputAttributeDescription>> Layout;
	};

	struct PipelineConfigInfo
	{
		PipelineConfigInfo() = default;

		std::vector<VkVertexInputBindingDescription> BindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> AttributeDescriptions;
		VkPipelineViewportStateCreateInfo ViewportInfo;
		VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo;
		VkPipelineRasterizationStateCreateInfo RasterizationInfo;
		VkPipelineMultisampleStateCreateInfo MultisampleInfo;
		VkPipelineColorBlendAttachmentState ColorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo ColorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo DepthStencilInfo;
		std::vector<VkDynamicState> DynamicStateEnables;
		VkPipelineDynamicStateCreateInfo DynamicStateInfo;
		VkPipelineLayout PipelineLayout = nullptr;
		std::shared_ptr<VulkanRenderPass> RenderPass;
		uint32_t SubPass = 0;
	};

	class VulkanPipeline
	{
	public:
		VulkanPipeline() = default;
		VulkanPipeline(const PipelineSpecification& spec);
		VulkanPipeline(PipelineConfigInfo& pipelineInfo, const std::string& vertFilepath,
			const std::string& fragFilepath, const std::string& geomFilepath = "");

		~VulkanPipeline();

		static void DefaultPipelineConfigInfo(PipelineConfigInfo& pipelineConfigInfo);
		static void EnableAlphaBlending(PipelineConfigInfo& pipelineConfigInfo);

		void Bind(VkCommandBuffer commandBuffer);
	private:
		void CreateGraphicsPipeline(std::shared_ptr<Shader> shader, const PipelineConfigInfo& pipelineInfo);
		void CreateGraphicsPipeline(std::shared_ptr<Shader> shader);
		void CreateShaderModule(const std::string& shaderSource, VkShaderModule* shaderModule);
		void CreateShaderModule(const std::vector<uint32_t>& shaderSource, VkShaderModule* shaderModule);
		void CreatePipelineCache();
	private:
		PipelineSpecification m_Specification;

		VkPipeline m_GraphicsPipeline;
		VkShaderModule m_vertShaderModule, m_fragShaderModule, m_geomShaderModule = nullptr;
		std::shared_ptr<Shader> m_Shader;
	};

}