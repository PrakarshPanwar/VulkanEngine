#include "vulkanpch.h"
#include "VulkanPipeline.h"
#include "VulkanMesh.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

#include <filesystem>

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

		static VkSampleCountFlagBits VulkanSampleCount(uint32_t sampleCount)
		{
			switch (sampleCount)
			{
			case 1:  return VK_SAMPLE_COUNT_1_BIT;
			case 2:  return VK_SAMPLE_COUNT_2_BIT;
			case 4:  return VK_SAMPLE_COUNT_4_BIT;
			case 8:  return VK_SAMPLE_COUNT_8_BIT;
			case 16: return VK_SAMPLE_COUNT_16_BIT;
			case 32: return VK_SAMPLE_COUNT_32_BIT;
			case 64: return VK_SAMPLE_COUNT_64_BIT;
			default:
				VK_CORE_ASSERT(false, "Sample Bit not Supported! Choose Power of 2");
				return VK_SAMPLE_COUNT_1_BIT;
			}
		}

	}

	VulkanPipeline::VulkanPipeline(PipelineConfigInfo& pipelineInfo, const std::string& vertFilepath, const std::string& fragFilepath, const std::string& geomFilepath)
	{
		if (geomFilepath.empty())
		{
			std::filesystem::path VertexFilePath = vertFilepath;
			std::filesystem::path FragmentFilePath = fragFilepath;

			m_Shader = std::make_shared<Shader>(vertFilepath, fragFilepath);
			CreateGraphicsPipeline(m_Shader, pipelineInfo);
		}

		else
		{
			std::filesystem::path VertexFilePath = vertFilepath;
			std::filesystem::path FragmentFilePath = fragFilepath;
			std::filesystem::path GeometryFilepath = geomFilepath;

			m_Shader = std::make_shared<Shader>(vertFilepath, fragFilepath, geomFilepath);
			CreateGraphicsPipeline(m_Shader, pipelineInfo);
		}
	}

	VulkanPipeline::VulkanPipeline(const PipelineSpecification& spec)
		: m_Specification(spec)
	{
	}

	VulkanPipeline::~VulkanPipeline()
	{
		auto device = VulkanContext::GetCurrentDevice();

		vkDestroyShaderModule(device->GetVulkanDevice(), m_vertShaderModule, nullptr);
		vkDestroyShaderModule(device->GetVulkanDevice(), m_fragShaderModule, nullptr);

		if (m_geomShaderModule != VK_NULL_HANDLE)
			vkDestroyShaderModule(device->GetVulkanDevice(), m_geomShaderModule, nullptr);

		vkDestroyPipeline(device->GetVulkanDevice(), m_GraphicsPipeline, nullptr);
	}

	void VulkanPipeline::CreateGraphicsPipeline(std::shared_ptr<Shader> shader, const PipelineConfigInfo& pipelineInfo)
	{
		VK_CORE_ASSERT(pipelineInfo.RenderPass != VK_NULL_HANDLE, "Failed to Create Graphics Pipeline: Render Pass is VK_NULL_HANDLE");
		VK_CORE_ASSERT(pipelineInfo.PipelineLayout != VK_NULL_HANDLE, "Failed to Create Graphics Pipeline: Pipeline Layout is VK_NULL_HANDLE");

		auto device = VulkanContext::GetCurrentDevice();
		auto& shaderSources = shader->GetShaderModules();

		CreateShaderModule(shaderSources[(uint32_t)ShaderType::Vertex], &m_vertShaderModule);
		CreateShaderModule(shaderSources[(uint32_t)ShaderType::Fragment], &m_fragShaderModule);

		if (shader->CheckIfGeometryShaderExists())
			CreateShaderModule(shaderSources[(uint32_t)ShaderType::Geometry], &m_geomShaderModule);

		VkPipelineShaderStageCreateInfo* shaderStages;

		const uint32_t shaderStageCount = shader->CheckIfGeometryShaderExists() ? 3 : 2;
		shaderStages = new VkPipelineShaderStageCreateInfo[shaderStageCount];

		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStages[0].module = m_vertShaderModule;
		shaderStages[0].pName = "main";
		shaderStages[0].flags = 0;
		shaderStages[0].pNext = nullptr;
		shaderStages[0].pSpecializationInfo = nullptr;

		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStages[1].module = m_fragShaderModule;
		shaderStages[1].pName = "main";
		shaderStages[1].flags = 0;
		shaderStages[1].pNext = nullptr;
		shaderStages[1].pSpecializationInfo = nullptr;

		if (shader->CheckIfGeometryShaderExists())
		{
			shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[2].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			shaderStages[2].module = m_geomShaderModule;
			shaderStages[2].pName = "main";
			shaderStages[2].flags = 0;
			shaderStages[2].pNext = nullptr;
			shaderStages[2].pSpecializationInfo = nullptr;
		}

		auto& bindingDescriptions = pipelineInfo.BindingDescriptions;
		auto& attributeDescriptions = pipelineInfo.AttributeDescriptions;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

		VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
		graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipelineInfo.stageCount = shaderStageCount;
		graphicsPipelineInfo.pStages = shaderStages;
		graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
		graphicsPipelineInfo.pInputAssemblyState = &pipelineInfo.InputAssemblyInfo;
		graphicsPipelineInfo.pViewportState = &pipelineInfo.ViewportInfo;
		graphicsPipelineInfo.pRasterizationState = &pipelineInfo.RasterizationInfo;
		graphicsPipelineInfo.pColorBlendState = &pipelineInfo.ColorBlendInfo;
		graphicsPipelineInfo.pDepthStencilState = &pipelineInfo.DepthStencilInfo;
		graphicsPipelineInfo.pMultisampleState = &pipelineInfo.MultisampleInfo;
		graphicsPipelineInfo.pDynamicState = &pipelineInfo.DynamicStateInfo;

		graphicsPipelineInfo.layout = pipelineInfo.PipelineLayout;
		graphicsPipelineInfo.renderPass = pipelineInfo.RenderPass->GetRenderPass();
		graphicsPipelineInfo.subpass = pipelineInfo.SubPass;

		graphicsPipelineInfo.basePipelineIndex = -1;
		graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device->GetVulkanDevice(),
			VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &m_GraphicsPipeline),
			"Failed to Create Graphics Pipeline!");

		delete[] shaderStages;
	}

	// For Pipeline Specification
	void VulkanPipeline::CreateGraphicsPipeline(std::shared_ptr<Shader> shader)
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto& shaderSources = shader->GetShaderModules();

		m_vertShaderModule = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::Vertex]);
		m_fragShaderModule = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::Fragment]);

		if (shader->CheckIfGeometryShaderExists())
			m_geomShaderModule = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::Geometry]);

		const uint32_t shaderStageCount = shader->CheckIfGeometryShaderExists() ? 3 : 2;
		VkPipelineShaderStageCreateInfo* shaderStages = new VkPipelineShaderStageCreateInfo[shaderStageCount];

		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStages[0].module = m_vertShaderModule;
		shaderStages[0].pName = "main";
		shaderStages[0].flags = 0;
		shaderStages[0].pNext = nullptr;
		shaderStages[0].pSpecializationInfo = nullptr;

		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStages[1].module = m_fragShaderModule;
		shaderStages[1].pName = "main";
		shaderStages[1].flags = 0;
		shaderStages[1].pNext = nullptr;
		shaderStages[1].pSpecializationInfo = nullptr;

		if (shader->CheckIfGeometryShaderExists())
		{
			shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[2].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			shaderStages[2].module = m_geomShaderModule;
			shaderStages[2].pName = "main";
			shaderStages[2].flags = 0;
			shaderStages[2].pNext = nullptr;
			shaderStages[2].pSpecializationInfo = nullptr;
		}

		delete[] shaderStages;
	}

	void VulkanPipeline::CreateShaderModule(const std::string& shaderSource, VkShaderModule* shaderModule)
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shaderSource.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderSource.data());

		VK_CHECK_RESULT(vkCreateShaderModule(device->GetVulkanDevice(), &createInfo, nullptr, shaderModule), "Failed to Create Shader Module!");
	}

	void VulkanPipeline::CreateShaderModule(const std::vector<uint32_t>& shaderSource, VkShaderModule* shaderModule)
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shaderSource.size() * sizeof(uint32_t);
		createInfo.pCode = shaderSource.data();

		VK_CHECK_RESULT(vkCreateShaderModule(device->GetVulkanDevice(), &createInfo, nullptr, shaderModule), "Failed to Create Shader Module!");
	}

	void VulkanPipeline::CreatePipelineCache()
	{
		// TODO: Caching could benefit greatly in performance
		VkPipelineCacheCreateInfo cacheInfo{};
		cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		cacheInfo.initialDataSize = 0;
		cacheInfo.pNext = nullptr;
		cacheInfo.pInitialData = nullptr;
	}

	void VulkanPipeline::DefaultPipelineConfigInfo(PipelineConfigInfo& pipelineConfigInfo)
	{
		pipelineConfigInfo.InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		pipelineConfigInfo.InputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineConfigInfo.InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		pipelineConfigInfo.ViewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		pipelineConfigInfo.ViewportInfo.viewportCount = 1;
		pipelineConfigInfo.ViewportInfo.pViewports = nullptr;
		pipelineConfigInfo.ViewportInfo.scissorCount = 1;
		pipelineConfigInfo.ViewportInfo.pScissors = nullptr;

		pipelineConfigInfo.RasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		pipelineConfigInfo.RasterizationInfo.depthClampEnable = VK_FALSE;
		pipelineConfigInfo.RasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		pipelineConfigInfo.RasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		pipelineConfigInfo.RasterizationInfo.lineWidth = 1.0f;
		pipelineConfigInfo.RasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		pipelineConfigInfo.RasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		pipelineConfigInfo.RasterizationInfo.depthBiasEnable = VK_FALSE;
		pipelineConfigInfo.RasterizationInfo.depthBiasConstantFactor = 0.0f;
		pipelineConfigInfo.RasterizationInfo.depthBiasClamp = 0.0f;
		pipelineConfigInfo.RasterizationInfo.depthBiasSlopeFactor = 0.0f;

		auto Framebuffer = pipelineConfigInfo.RenderPass->GetSpecification().TargetFramebuffer;
		const auto sampleCount = Utils::VulkanSampleCount(Framebuffer->GetSpecification().Samples);
		pipelineConfigInfo.MultisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		pipelineConfigInfo.MultisampleInfo.sampleShadingEnable = VK_FALSE;
		pipelineConfigInfo.MultisampleInfo.rasterizationSamples = sampleCount;
		pipelineConfigInfo.MultisampleInfo.minSampleShading = 1.0f;
		pipelineConfigInfo.MultisampleInfo.pSampleMask = nullptr;
		pipelineConfigInfo.MultisampleInfo.alphaToCoverageEnable = VK_FALSE;
		pipelineConfigInfo.MultisampleInfo.alphaToOneEnable = VK_FALSE;

		pipelineConfigInfo.ColorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		pipelineConfigInfo.ColorBlendAttachment.blendEnable = VK_FALSE;
		pipelineConfigInfo.ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
		pipelineConfigInfo.ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
		pipelineConfigInfo.ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
		pipelineConfigInfo.ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
		pipelineConfigInfo.ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
		pipelineConfigInfo.ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

		pipelineConfigInfo.ColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		pipelineConfigInfo.ColorBlendInfo.logicOpEnable = VK_FALSE;
		pipelineConfigInfo.ColorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
		pipelineConfigInfo.ColorBlendInfo.attachmentCount = 1;
		pipelineConfigInfo.ColorBlendInfo.pAttachments = &pipelineConfigInfo.ColorBlendAttachment;
		pipelineConfigInfo.ColorBlendInfo.blendConstants[0] = 0.0f;  // Optional
		pipelineConfigInfo.ColorBlendInfo.blendConstants[1] = 0.0f;  // Optional
		pipelineConfigInfo.ColorBlendInfo.blendConstants[2] = 0.0f;  // Optional
		pipelineConfigInfo.ColorBlendInfo.blendConstants[3] = 0.0f;  // Optional

		pipelineConfigInfo.DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		pipelineConfigInfo.DepthStencilInfo.depthTestEnable = VK_TRUE;
		pipelineConfigInfo.DepthStencilInfo.depthWriteEnable = VK_TRUE;
		pipelineConfigInfo.DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		pipelineConfigInfo.DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		pipelineConfigInfo.DepthStencilInfo.minDepthBounds = 0.0f;  // Optional
		pipelineConfigInfo.DepthStencilInfo.maxDepthBounds = 1.0f;  // Optional
		pipelineConfigInfo.DepthStencilInfo.stencilTestEnable = VK_FALSE;
		pipelineConfigInfo.DepthStencilInfo.front = {};  // Optional
		pipelineConfigInfo.DepthStencilInfo.back = {};   // Optional

		pipelineConfigInfo.DynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		pipelineConfigInfo.DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		pipelineConfigInfo.DynamicStateInfo.pDynamicStates = pipelineConfigInfo.DynamicStateEnables.data();
		pipelineConfigInfo.DynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(pipelineConfigInfo.DynamicStateEnables.size());
		pipelineConfigInfo.DynamicStateInfo.flags = 0;

		pipelineConfigInfo.BindingDescriptions = Vertex::GetBindingDescriptions();
		pipelineConfigInfo.AttributeDescriptions = Vertex::GetAttributeDescriptions();
	}

	void VulkanPipeline::EnableAlphaBlending(PipelineConfigInfo& pipelineConfigInfo)
	{
		pipelineConfigInfo.ColorBlendAttachment.blendEnable = VK_TRUE;
		pipelineConfigInfo.ColorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		pipelineConfigInfo.ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		pipelineConfigInfo.ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		pipelineConfigInfo.ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		pipelineConfigInfo.ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		pipelineConfigInfo.ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		pipelineConfigInfo.ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	void VulkanPipeline::Bind(VkCommandBuffer commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
	}

}