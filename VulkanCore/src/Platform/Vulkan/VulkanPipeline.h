#pragma once
#include "VulkanDevice.h"
#include "VulkanCore/Core/Shader.h"
#include "VulkanRenderPass.h"
#include "VulkanCore/Renderer/Buffer.h"

namespace VulkanCore {

	struct PipelineSpecification
	{
		PipelineSpecification() = default;

		std::string DebugName;
		std::shared_ptr<Shader> pShader;
		std::shared_ptr<VulkanRenderPass> RenderPass;
		bool BackfaceCulling = false;
		bool DepthTest = true;
		bool DepthWrite = true;
		bool Blend = false;
		VertexBufferLayout Layout{};
		VertexBufferLayout InstanceLayout{};
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

	struct PipelineConfiguration
	{
		PipelineConfiguration() = default;

		VkPipelineViewportStateCreateInfo ViewportInfo;
		VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo;
		VkPipelineRasterizationStateCreateInfo RasterizationInfo;
		VkPipelineMultisampleStateCreateInfo MultisampleInfo;
		std::vector<VkPipelineColorBlendAttachmentState> ColorBlendAttachments;
		VkPipelineColorBlendStateCreateInfo ColorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo DepthStencilInfo;
		std::vector<VkDynamicState> DynamicStateEnables;
		VkPipelineDynamicStateCreateInfo DynamicStateInfo;
		uint32_t Subpass = 0;
	};

	class VulkanPipeline
	{
	public:
		VulkanPipeline() = default;
		VulkanPipeline(const PipelineSpecification& spec);
		VulkanPipeline(PipelineConfigInfo& pipelineInfo, const std::string& vertFilepath,
			const std::string& fragFilepath, const std::string& geomFilepath = "");

		~VulkanPipeline();

		static void SetDefaultPipelineConfiguration(PipelineConfigInfo& pipelineConfigInfo);
		static void EnableAlphaBlending(PipelineConfigInfo& pipelineConfigInfo);

		void Bind(VkCommandBuffer commandBuffer);
		void SetPushConstants(VkCommandBuffer cmdBuf, void* pcData, size_t size);

		inline VkPipelineLayout GetVulkanPipelineLayout() const { return m_PipelineLayout; }
		inline PipelineSpecification GetSpecification() const { return m_Specification; }
		inline std::shared_ptr<VulkanDescriptorSetLayout> GetDescriptorSetLayout(uint32_t index = 0) const { return m_DescriptorSetLayout[index]; }
	private:
		void CreateGraphicsPipeline(std::shared_ptr<Shader> shader, const PipelineConfigInfo& pipelineInfo);
		void CreateGraphicsPipeline();
		void CreateShaderModule(const std::string& shaderSource, VkShaderModule* shaderModule);
		void CreateShaderModule(const std::vector<uint32_t>& shaderSource, VkShaderModule* shaderModule);
		void CreatePipelineCache();
	private:
		PipelineSpecification m_Specification;

		VkPipeline m_GraphicsPipeline;
		VkPipelineLayout m_PipelineLayout = nullptr;
		VkShaderModule m_vertShaderModule, m_fragShaderModule, m_geomShaderModule = nullptr;
		std::shared_ptr<Shader> m_Shader;
		std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> m_DescriptorSetLayout;
	};

}