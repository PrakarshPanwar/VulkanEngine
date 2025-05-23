#include "vulkanpch.h"
#include "VulkanPipeline.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Mesh/Mesh.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "VulkanShader.h"
#include "VulkanRenderPass.h"
#include "Utils/ImageUtils.h"

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
			pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
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

		static VkPipelineLayout CreatePipelineLayout(std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> descriptorSetLayouts, size_t pushConstantSize = 0)
		{
			auto device = VulkanContext::GetCurrentDevice();

			VkPushConstantRange pushConstantRange{};
			pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			pushConstantRange.offset = 0;
			pushConstantRange.size = (uint32_t)pushConstantSize;

			std::vector<VkDescriptorSetLayout> vulkanDescriptorSetsLayout{};
			for (auto& descriptorSetLayout : descriptorSetLayouts)
				vulkanDescriptorSetsLayout.push_back(descriptorSetLayout->GetVulkanDescriptorSetLayout());

			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
			pipelineLayoutInfo.pSetLayouts = vulkanDescriptorSetsLayout.data();
			pipelineLayoutInfo.pushConstantRangeCount = pushConstantSize == 0 ? 0 : 1;
			pipelineLayoutInfo.pPushConstantRanges = pushConstantSize == 0 ? nullptr : &pushConstantRange;

			VkPipelineLayout pipelineLayout;
			VK_CHECK_RESULT(vkCreatePipelineLayout(device->GetVulkanDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout), "Failed to Create Pipeline Layout!");
			return pipelineLayout;
		}

		static VkPrimitiveTopology VulkanPrimitiveTopology(PrimitiveTopology primitiveTopology)
		{
			switch (primitiveTopology)
			{
			case PrimitiveTopology::LineList:	  return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			case PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			case PrimitiveTopology::PatchList:	  return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
			default:
				VK_CORE_ASSERT(false, "Primitive Topology not supported!");
				return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
			}
		}

		static VkCompareOp VulkanCompareOp(CompareOp compareOp)
		{
			switch (compareOp)
			{
			case CompareOp::None:		 return VK_COMPARE_OP_NEVER;
			case CompareOp::Less:		 return VK_COMPARE_OP_LESS;
			case CompareOp::LessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
			default:
				VK_CORE_ASSERT(false, "Compare Op not supported!");
				return VK_COMPARE_OP_LESS;
			}
		}

		static VkCullModeFlags VulkanCullModeFlags(CullMode cullMode)
		{
			switch (cullMode)
			{
			case CullMode::None:		 return VK_CULL_MODE_NONE;
			case CullMode::Front:		 return VK_CULL_MODE_FRONT_BIT;
			case CullMode::Back:		 return VK_CULL_MODE_BACK_BIT;
			case CullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
			default:
				VK_CORE_ASSERT(false, "Culling Mode not supported!");
				return VK_CULL_MODE_NONE;
			}
		}

		static PipelineConfiguration GetPipelineConfiguration(const PipelineSpecification& spec)
		{
			PipelineConfiguration pipelineConfig{};

			pipelineConfig.InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			pipelineConfig.InputAssemblyInfo.topology = Utils::VulkanPrimitiveTopology(spec.Topology);
			pipelineConfig.InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

			pipelineConfig.ViewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			pipelineConfig.ViewportInfo.viewportCount = 0;
			pipelineConfig.ViewportInfo.pViewports = nullptr;
			pipelineConfig.ViewportInfo.scissorCount = 0;
			pipelineConfig.ViewportInfo.pScissors = nullptr;

			pipelineConfig.RasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			pipelineConfig.RasterizationInfo.depthClampEnable = spec.DepthClamp ? VK_TRUE : VK_FALSE;
			pipelineConfig.RasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
			pipelineConfig.RasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
			pipelineConfig.RasterizationInfo.lineWidth = 1.0f;
			pipelineConfig.RasterizationInfo.cullMode = Utils::VulkanCullModeFlags(spec.CullingMode);
			pipelineConfig.RasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
			pipelineConfig.RasterizationInfo.depthBiasEnable = VK_FALSE;
			pipelineConfig.RasterizationInfo.depthBiasConstantFactor = 0.0f;
			pipelineConfig.RasterizationInfo.depthBiasClamp = 0.0f;
			pipelineConfig.RasterizationInfo.depthBiasSlopeFactor = 0.0f;

			auto Framebuffer = spec.pRenderPass->GetSpecification().TargetFramebuffer;
			const auto sampleCount = VulkanSampleCount(Framebuffer->GetSpecification().Samples);
			pipelineConfig.MultisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			pipelineConfig.MultisampleInfo.sampleShadingEnable = VK_FALSE;
			pipelineConfig.MultisampleInfo.rasterizationSamples = sampleCount;
			pipelineConfig.MultisampleInfo.minSampleShading = 1.0f;
			pipelineConfig.MultisampleInfo.pSampleMask = nullptr;
			pipelineConfig.MultisampleInfo.alphaToCoverageEnable = VK_FALSE;
			pipelineConfig.MultisampleInfo.alphaToOneEnable = VK_FALSE;

			// TODO: We have to add multiple blending attachments as
			// there could multiple be RenderPass attachments
			pipelineConfig.ColorBlendAttachments.resize(Framebuffer->GetColorAttachmentsTextureSpec().size());

			for (int i = 0; i < Framebuffer->GetColorAttachmentsTextureSpec().size(); i++)
			{
				pipelineConfig.ColorBlendAttachments[i].colorWriteMask =
					VK_COLOR_COMPONENT_R_BIT |
					VK_COLOR_COMPONENT_G_BIT |
					VK_COLOR_COMPONENT_B_BIT |
					VK_COLOR_COMPONENT_A_BIT;

				if (spec.Blend)
				{
					pipelineConfig.ColorBlendAttachments[i].blendEnable = VK_TRUE;
					pipelineConfig.ColorBlendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
					pipelineConfig.ColorBlendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
					pipelineConfig.ColorBlendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
					pipelineConfig.ColorBlendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
					pipelineConfig.ColorBlendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
					pipelineConfig.ColorBlendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;
				}
				else
				{
					pipelineConfig.ColorBlendAttachments[i].blendEnable = VK_FALSE;
					pipelineConfig.ColorBlendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
					pipelineConfig.ColorBlendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
					pipelineConfig.ColorBlendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;              // Optional
					pipelineConfig.ColorBlendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
					pipelineConfig.ColorBlendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
					pipelineConfig.ColorBlendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;              // Optional
				}
			}

			pipelineConfig.ColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			pipelineConfig.ColorBlendInfo.logicOpEnable = VK_FALSE;
			pipelineConfig.ColorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
			pipelineConfig.ColorBlendInfo.attachmentCount = (uint32_t)pipelineConfig.ColorBlendAttachments.size();
			pipelineConfig.ColorBlendInfo.pAttachments = pipelineConfig.ColorBlendAttachments.data();
			pipelineConfig.ColorBlendInfo.blendConstants[0] = 0.0f;  // Optional
			pipelineConfig.ColorBlendInfo.blendConstants[1] = 0.0f;  // Optional
			pipelineConfig.ColorBlendInfo.blendConstants[2] = 0.0f;  // Optional
			pipelineConfig.ColorBlendInfo.blendConstants[3] = 0.0f;  // Optional

			pipelineConfig.TessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
			pipelineConfig.TessellationInfo.patchControlPoints = spec.PatchControlPoints;

			pipelineConfig.DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			pipelineConfig.DepthStencilInfo.depthTestEnable = spec.DepthTest ? VK_TRUE : VK_FALSE;
			pipelineConfig.DepthStencilInfo.depthWriteEnable = spec.DepthWrite ? VK_TRUE : VK_FALSE;
			pipelineConfig.DepthStencilInfo.depthCompareOp = Utils::VulkanCompareOp(spec.DepthCompareOp);
			pipelineConfig.DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
			pipelineConfig.DepthStencilInfo.minDepthBounds = 0.0f;  // Optional
			pipelineConfig.DepthStencilInfo.maxDepthBounds = 1.0f;  // Optional
			pipelineConfig.DepthStencilInfo.stencilTestEnable = VK_FALSE;
			pipelineConfig.DepthStencilInfo.front = {};  // Optional
			pipelineConfig.DepthStencilInfo.back = {};   // Optional

			pipelineConfig.DynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT, VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT };
			pipelineConfig.DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			pipelineConfig.DynamicStateInfo.pDynamicStates = pipelineConfig.DynamicStateEnables.data();
			pipelineConfig.DynamicStateInfo.dynamicStateCount = (uint32_t)pipelineConfig.DynamicStateEnables.size();
			pipelineConfig.DynamicStateInfo.flags = 0;

			return pipelineConfig;
		}

		static VkFormat GetVulkanFormat(ShaderDataType type)
		{
			switch (type)
			{
			case ShaderDataType::None:	 return VK_FORMAT_UNDEFINED;
			case ShaderDataType::Float:	 return VK_FORMAT_R32_SFLOAT;
			case ShaderDataType::Float2: return VK_FORMAT_R32G32_SFLOAT;
			case ShaderDataType::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
			case ShaderDataType::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
			case ShaderDataType::Int:	 return VK_FORMAT_R32_SINT;
			case ShaderDataType::Int2:	 return VK_FORMAT_R32G32_SINT;
			case ShaderDataType::Int3:	 return VK_FORMAT_R32G32B32_SINT;
			case ShaderDataType::Int4:	 return VK_FORMAT_R32G32B32A32_SINT;
			case ShaderDataType::Bool:	 return VK_FORMAT_R8_SINT;
			default:
				VK_CORE_ASSERT(false, "Invalid or Not Supported Format!");
				return VK_FORMAT_UNDEFINED;
			}
		}

		static std::vector<VkVertexInputBindingDescription> GetVulkanBindingDescription(const PipelineSpecification& spec)
		{
			std::vector<VkVertexInputBindingDescription> bindingDescriptions;

			if (!spec.Layout.GetElements().empty())
			{
				auto& vertexBindingDescription = bindingDescriptions.emplace_back();
				vertexBindingDescription.binding = 0;
				vertexBindingDescription.stride = spec.Layout.GetStride();
				vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			}

			if (!spec.InstanceLayout.GetElements().empty())
			{
				auto& instanceBindingDescription = bindingDescriptions.emplace_back();
				instanceBindingDescription.binding = 1;
				instanceBindingDescription.stride = spec.InstanceLayout.GetStride();
				instanceBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
			}

			return bindingDescriptions;
		}

		static std::vector<VkVertexInputAttributeDescription> GetVulkanAttributeDescription(const PipelineSpecification& spec)
		{
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

			uint32_t index = 0;
			auto& bufferElements = spec.Layout.GetElements();
			for (auto& bufferElement : bufferElements)
			{
				auto& attribElement = attributeDescriptions.emplace_back();
				attribElement.location = index++;
				attribElement.binding = 0;
				attribElement.offset = bufferElement.Offset;
				attribElement.format = GetVulkanFormat(bufferElement.Type);
			}

			auto& instanceBufferElements = spec.InstanceLayout.GetElements();
			for (auto& bufferElement : instanceBufferElements)
			{
				auto& attribElement = attributeDescriptions.emplace_back();
				attribElement.location = index++;
				attribElement.binding = 1;
				attribElement.offset = bufferElement.Offset;
				attribElement.format = GetVulkanFormat(bufferElement.Type);
			}

			return attributeDescriptions;
		}

	}

	VulkanPipeline::VulkanPipeline(const PipelineSpecification& spec)
		: m_Specification(spec)
	{
		Renderer::Submit([this]
		{
			InvalidateGraphicsPipeline();
		});
	}

	VulkanPipeline::~VulkanPipeline()
	{
		Release();
	}

	void VulkanPipeline::InvalidateGraphicsPipeline()
	{
		auto device = VulkanContext::GetCurrentDevice();

		auto shader = std::static_pointer_cast<VulkanShader>(m_Specification.pShader);
		auto& shaderSources = shader->GetShaderModules();

		m_ShaderModules[ShaderType::Vertex] = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::Vertex]);
		m_ShaderModules[ShaderType::Fragment] = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::Fragment]);

		uint32_t shaderStageCount = 2;
		if (shader->HasTessellationShaders())
			shaderStageCount = 4;
		else if (shader->HasGeometryShader())
			shaderStageCount = 3;

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages(shaderStageCount);

		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStages[0].module = m_ShaderModules[ShaderType::Vertex];
		shaderStages[0].pName = "main";
		shaderStages[0].flags = 0;
		shaderStages[0].pNext = nullptr;
		shaderStages[0].pSpecializationInfo = nullptr;

		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStages[1].module = m_ShaderModules[ShaderType::Fragment];
		shaderStages[1].pName = "main";
		shaderStages[1].flags = 0;
		shaderStages[1].pNext = nullptr;
		shaderStages[1].pSpecializationInfo = nullptr;

		if (shader->HasGeometryShader())
		{
			m_ShaderModules[ShaderType::Geometry] = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::Geometry]);

			shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[2].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			shaderStages[2].module = m_ShaderModules[ShaderType::Geometry];
			shaderStages[2].pName = "main";
			shaderStages[2].flags = 0;
			shaderStages[2].pNext = nullptr;
			shaderStages[2].pSpecializationInfo = nullptr;
		}

		if (shader->HasTessellationShaders())
		{
			m_ShaderModules[ShaderType::TessellationControl] = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::TessellationControl]);

			shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[2].stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			shaderStages[2].module = m_ShaderModules[ShaderType::TessellationControl];
			shaderStages[2].pName = "main";
			shaderStages[2].flags = 0;
			shaderStages[2].pNext = nullptr;
			shaderStages[2].pSpecializationInfo = nullptr;

			m_ShaderModules[ShaderType::TessellationEvaluation] = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::TessellationEvaluation]);

			shaderStages[3].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[3].stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			shaderStages[3].module = m_ShaderModules[ShaderType::TessellationEvaluation];
			shaderStages[3].pName = "main";
			shaderStages[3].flags = 0;
			shaderStages[3].pNext = nullptr;
			shaderStages[3].pSpecializationInfo = nullptr;
		}

		auto bindingDescriptions = Utils::GetVulkanBindingDescription(m_Specification);
		auto attributeDescriptions = Utils::GetVulkanAttributeDescription(m_Specification);

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

		m_DescriptorSetLayout = shader->CreateAllDescriptorSetsLayout();
		m_PipelineLayout = Utils::CreatePipelineLayout(m_DescriptorSetLayout, shader->GetPushConstantSize());

		auto pipelineInfo = Utils::GetPipelineConfiguration(m_Specification);

		VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
		graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipelineInfo.stageCount = shaderStageCount;
		graphicsPipelineInfo.pStages = shaderStages.data();
		graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
		graphicsPipelineInfo.pInputAssemblyState = &pipelineInfo.InputAssemblyInfo;
		graphicsPipelineInfo.pTessellationState = m_Specification.PatchControlPoints > 0 ? &pipelineInfo.TessellationInfo : nullptr;
		graphicsPipelineInfo.pViewportState = &pipelineInfo.ViewportInfo;
		graphicsPipelineInfo.pRasterizationState = &pipelineInfo.RasterizationInfo;
		graphicsPipelineInfo.pColorBlendState = &pipelineInfo.ColorBlendInfo;
		graphicsPipelineInfo.pDepthStencilState = &pipelineInfo.DepthStencilInfo;
		graphicsPipelineInfo.pMultisampleState = &pipelineInfo.MultisampleInfo;
		graphicsPipelineInfo.pDynamicState = &pipelineInfo.DynamicStateInfo;
		graphicsPipelineInfo.layout = m_PipelineLayout;
		graphicsPipelineInfo.renderPass = std::dynamic_pointer_cast<VulkanRenderPass>(m_Specification.pRenderPass)->GetVulkanRenderPass();
		graphicsPipelineInfo.subpass = pipelineInfo.Subpass;
		graphicsPipelineInfo.basePipelineIndex = -1;
		graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(
			device->GetVulkanDevice(),
			VK_NULL_HANDLE,
			1,
			&graphicsPipelineInfo,
			nullptr,
			&m_GraphicsPipeline),
			"Failed to Create Graphics Pipeline!");

		VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(), VK_OBJECT_TYPE_PIPELINE, m_Specification.DebugName, m_GraphicsPipeline);
	}

	// TODO: Caching could benefit greatly in performance
	void VulkanPipeline::CreatePipelineCache()
	{
		VkPipelineCacheCreateInfo cacheInfo{};
		cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		cacheInfo.initialDataSize = 0;
		cacheInfo.pNext = nullptr;
		cacheInfo.pInitialData = nullptr;
	}

	void VulkanPipeline::Release()
	{
		Renderer::SubmitResourceFree([pipeline = m_GraphicsPipeline, layout = m_PipelineLayout, shaderModules = m_ShaderModules]
		{
			auto device = VulkanContext::GetCurrentDevice();

			for (auto&& [stage, module] : shaderModules)
				vkDestroyShaderModule(device->GetVulkanDevice(), module, nullptr);

			if (layout)
				vkDestroyPipelineLayout(device->GetVulkanDevice(), layout, nullptr);

			vkDestroyPipeline(device->GetVulkanDevice(), pipeline, nullptr);
		});

		m_GraphicsPipeline = nullptr;
	}

	void VulkanPipeline::Bind(VkCommandBuffer commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
	}

	void VulkanPipeline::SetPushConstants(VkCommandBuffer cmdBuf, void* pcData, size_t size)
	{
		vkCmdPushConstants(cmdBuf,
			m_PipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			(uint32_t)size,
			pcData);
	}

	void VulkanPipeline::ReloadPipeline()
	{
		auto shader = m_Specification.pShader;
		if (shader->GetReloadFlag())
		{
			Release();
			InvalidateGraphicsPipeline();
			shader->ResetReloadFlag();
		}
	}

}
