#pragma once
#include "VulkanDevice.h"
#include "VulkanCore/Core/Shader.h"

namespace VulkanCore {

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
		VkRenderPass RenderPass = nullptr;
		uint32_t SubPass = 0;
	};

	class VulkanPipeline
	{
	public:
		VulkanPipeline() = default;
		VulkanPipeline(VulkanDevice& device, PipelineConfigInfo& pipelineInfo, 
			const std::string& vertFilepath, const std::string& fragFilepath, const std::string& geomFilepath = "");

		~VulkanPipeline();
		static void DefaultPipelineConfigInfo(PipelineConfigInfo& pipelineConfigInfo);
		static void EnableAlphaBlending(PipelineConfigInfo& pipelineConfigInfo);

		void Bind(VkCommandBuffer commandBuffer);
	private:
		std::string ReadFile(const std::string& filepath);
		void CreateGraphicsPipeline(const PipelineConfigInfo& pipelineInfo, 
			const std::string& vsfilepath, const std::string& fsfilepath, const std::string& gsfilepath = "");
		void CreateGraphicsPipeline(std::shared_ptr<Shader> shader, const PipelineConfigInfo& pipelineInfo);
		void CreateShaderModule(const std::string& shaderSource, VkShaderModule* shaderModule);
		void CreateShaderModule(const std::vector<uint32_t>& shaderSource, VkShaderModule* shaderModule);
		void CreatePipelineCache(); //TODO
	private:
		VulkanDevice& m_VulkanDevice;
		VkPipeline m_GraphicsPipeline;
		VkShaderModule m_vertShaderModule, m_fragShaderModule, m_geomShaderModule = nullptr;
		std::shared_ptr<Shader> m_Shader;
	};

}