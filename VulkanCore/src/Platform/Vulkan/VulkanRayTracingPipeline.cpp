#include "vulkanpch.h"
#include "VulkanRayTracingPipeline.h"
#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "VulkanAllocator.h"

namespace VulkanCore {

	namespace Utils {

		static std::map<std::filesystem::path, ShaderType> s_ShaderExtensionMap = {
			{ ".rgen",  ShaderType::RayGeneration },
			{ ".rahit", ShaderType::RayAnyHit },
			{ ".rchit", ShaderType::RayClosestHit },
			{ ".rmiss", ShaderType::RayMiss },
			{ ".rint",  ShaderType::RayIntersection }
		};

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

		static VkPipelineLayout CreatePipelineLayout(std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> descriptorSetLayouts, size_t pushConstantSize = 0)
		{
			auto device = VulkanContext::GetCurrentDevice();

			VkPushConstantRange pushConstantRange{};
			pushConstantRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
			pushConstantRange.offset = 0;
			pushConstantRange.size = (uint32_t)pushConstantSize;

			std::vector<VkDescriptorSetLayout> vulkanDescriptorSetsLayout;
			for (auto& descriptorSetLayout : descriptorSetLayouts)
				vulkanDescriptorSetsLayout.push_back(descriptorSetLayout->GetVulkanDescriptorSetLayout());

			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = (uint32_t)vulkanDescriptorSetsLayout.size();
			pipelineLayoutInfo.pSetLayouts = vulkanDescriptorSetsLayout.data();
			pipelineLayoutInfo.pushConstantRangeCount = pushConstantSize == 0 ? 0 : 1;
			pipelineLayoutInfo.pPushConstantRanges = pushConstantSize == 0 ? nullptr : &pushConstantRange;

			VkPipelineLayout pipelineLayout;
			VK_CHECK_RESULT(vkCreatePipelineLayout(device->GetVulkanDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout), "Failed to Create Pipeline Layout!");
			return pipelineLayout;
		}

	}

	VulkanRayTracingPipeline::VulkanRayTracingPipeline(std::shared_ptr<ShaderBindingTable> shaderBindingTable, const std::string& debugName)
		: m_DebugName(debugName), m_ShaderBindingTable(shaderBindingTable)
	{
		InvalidateRayTracingPipeline();
	}

	VulkanRayTracingPipeline::~VulkanRayTracingPipeline()
	{
		Release();
	}

	void VulkanRayTracingPipeline::Release()
	{
		auto device = VulkanContext::GetCurrentDevice();

		// Destroy Shader Modules
		vkDestroyShaderModule(device->GetVulkanDevice(), m_RayGenShaderModule, nullptr);

		for (auto& rayClosestHitModule : m_RayClosestHitShaderModules)
			vkDestroyShaderModule(device->GetVulkanDevice(), rayClosestHitModule, nullptr);

		for (auto& rayAnyHitModule : m_RayAnyHitShaderModules)
			vkDestroyShaderModule(device->GetVulkanDevice(), rayAnyHitModule, nullptr);

		for (auto& rayIntersectionModule : m_RayIntersectionShaderModules)
			vkDestroyShaderModule(device->GetVulkanDevice(), rayIntersectionModule, nullptr);

		for (auto& rayMissModule : m_RayMissShaderModules)
			vkDestroyShaderModule(device->GetVulkanDevice(), rayMissModule, nullptr);

		if (m_PipelineLayout)
			vkDestroyPipelineLayout(device->GetVulkanDevice(), m_PipelineLayout, nullptr);

		// Destroy Pipeline
		vkDestroyPipeline(device->GetVulkanDevice(), m_RayTracingPipeline, nullptr);

		m_RayTracingPipeline = nullptr;
	}

	void VulkanRayTracingPipeline::Bind(VkCommandBuffer commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_RayTracingPipeline);
	}

	void VulkanRayTracingPipeline::SetPushConstants(VkCommandBuffer cmdBuf, void* pcData, size_t size)
	{
		vkCmdPushConstants(cmdBuf,
			m_PipelineLayout,
			VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
			0,
			(uint32_t)size,
			pcData);
	}

#if 0
	void VulkanRayTracingPipeline::CreateShaderBindingTable()
	{
		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("ShaderBindingTable");

		auto rayTracingPipelineProperties = device->GetPhysicalDeviceRayTracingPipelineProperties();
		const uint32_t shaderGroupBaseAlignment = rayTracingPipelineProperties.shaderGroupBaseAlignment;
		const uint32_t shaderGroupHandleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
		const uint32_t shaderGroupHandleAlignment = rayTracingPipelineProperties.shaderGroupHandleAlignment;
		const uint32_t shaderHandleSizeAligned = Utils::GetAlignment(shaderGroupHandleSize, shaderGroupHandleAlignment);
		const uint32_t groupCount = (uint32_t)m_ShaderGroups.size();
		const uint32_t sbtSize = groupCount * shaderHandleSizeAligned;

		// Get Shader Handles
		std::vector<uint8_t> shaderHandleStorage(sbtSize);
		VK_CHECK_RESULT(vkGetRayTracingShaderGroupHandlesKHR(
			device->GetVulkanDevice(),
			m_RayTracingPipeline,
			0,
			groupCount,
			sbtSize,
			shaderHandleStorage.data()),
			"Failed to Retrieve Shader Handles!");

		// Create SBT Buffer
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = shaderGroupBaseAlignment * groupCount;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		m_SBTMemAlloc = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, m_SBTBuffer);
		uint8_t* sbtBufferDst = allocator.MapMemory<uint8_t>(m_SBTMemAlloc);
		uint32_t offset = 0;

		// Ray Gen
		m_RayGenSBTInfo = Utils::GetSBTStridedDeviceAddressRegion(m_SBTBuffer);
		memcpy(sbtBufferDst, shaderHandleStorage.data(), shaderGroupBaseAlignment);
		sbtBufferDst += m_RayGenSBTInfo.size;
		offset += m_RayGenSBTInfo.size;

		// Closest Hit
		m_RayClosestHitSBTInfo = Utils::GetSBTStridedDeviceAddressRegion(m_SBTBuffer, offset);
		memcpy(sbtBufferDst, shaderHandleStorage.data() + shaderHandleSizeAligned, shaderGroupHandleSize);
		sbtBufferDst += m_RayClosestHitSBTInfo.size;
		offset += m_RayClosestHitSBTInfo.size;

		// Miss
		m_RayMissSBTInfo = Utils::GetSBTStridedDeviceAddressRegion(m_SBTBuffer, offset);
		memcpy(sbtBufferDst, shaderHandleStorage.data() + shaderHandleSizeAligned * 2, shaderGroupHandleSize);

		allocator.UnmapMemory(m_SBTMemAlloc);
	}
#endif

	void VulkanRayTracingPipeline::ReloadPipeline()
	{
		if (m_ShaderBindingTable->GetShader()->GetReloadFlag())
		{
			Release();
			InvalidateRayTracingPipeline();
			m_ShaderBindingTable->GetShader()->ResetReloadFlag();
		}
	}

	void VulkanRayTracingPipeline::InvalidateRayTracingPipeline()
	{
		auto device = VulkanContext::GetCurrentDevice();

		auto vulkanSBT = std::static_pointer_cast<VulkanShaderBindingTable>(m_ShaderBindingTable);
		auto shader = std::static_pointer_cast<VulkanRayTraceShader>(m_ShaderBindingTable->GetShader());
		auto& shaderSources = shader->GetShaderModules();

		// TODO: This is temporary
		for (auto&& [shaderPath, source] : shaderSources)
		{
			ShaderType stage = Utils::s_ShaderExtensionMap[shaderPath.extension()];

			switch (stage)
			{
			case ShaderType::RayGeneration:
				m_RayGenShaderModule = Utils::CreateShaderModule(source);
				break;
			case ShaderType::RayClosestHit:
			{
				VkShaderModule closestHitModule = Utils::CreateShaderModule(source);
				m_RayClosestHitShaderModules.push_back(closestHitModule);
				break;
			}
			case ShaderType::RayMiss:
			{
				VkShaderModule missModule = Utils::CreateShaderModule(source);
				m_RayMissShaderModules.push_back(missModule);
				break;
			}
			case ShaderType::RayIntersection:
			{
				VkShaderModule intersectionModule = Utils::CreateShaderModule(source);
				m_RayIntersectionShaderModules.push_back(intersectionModule);
				break;
			}
			default:
				break;
			}
		}

		size_t shaderStageCount = m_RayClosestHitShaderModules.size()
			+ m_RayMissShaderModules.size()
			+ m_RayIntersectionShaderModules.size() + 1;

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages{ shaderStageCount };

		// Ray Gen
		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		shaderStages[0].module = m_RayGenShaderModule;
		shaderStages[0].pName = "main";
		shaderStages[0].flags = 0;
		shaderStages[0].pNext = nullptr;
		shaderStages[0].pSpecializationInfo = nullptr;

		uint32_t stageIndex = 1;
		for (auto& rayClosestHitModule : m_RayClosestHitShaderModules)
		{
			shaderStages[stageIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[stageIndex].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			shaderStages[stageIndex].module = rayClosestHitModule;
			shaderStages[stageIndex].pName = "main";
			shaderStages[stageIndex].flags = 0;
			shaderStages[stageIndex].pNext = nullptr;
			shaderStages[stageIndex].pSpecializationInfo = nullptr;

			stageIndex++;
		}

		for (auto& rayAnyHitModule : m_RayAnyHitShaderModules)
		{
			shaderStages[stageIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[stageIndex].stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
			shaderStages[stageIndex].module = rayAnyHitModule;
			shaderStages[stageIndex].pName = "main";
			shaderStages[stageIndex].flags = 0;
			shaderStages[stageIndex].pNext = nullptr;
			shaderStages[stageIndex].pSpecializationInfo = nullptr;

			stageIndex++;
		}

		for (auto& rayIntersectionModule : m_RayIntersectionShaderModules)
		{
			shaderStages[stageIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[stageIndex].stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
			shaderStages[stageIndex].module = rayIntersectionModule;
			shaderStages[stageIndex].pName = "main";
			shaderStages[stageIndex].flags = 0;
			shaderStages[stageIndex].pNext = nullptr;
			shaderStages[stageIndex].pSpecializationInfo = nullptr;

			stageIndex++;
		}

		for (auto& rayMissModule : m_RayMissShaderModules)
		{
			shaderStages[stageIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[stageIndex].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
			shaderStages[stageIndex].module = rayMissModule;
			shaderStages[stageIndex].pName = "main";
			shaderStages[stageIndex].flags = 0;
			shaderStages[stageIndex].pNext = nullptr;
			shaderStages[stageIndex].pSpecializationInfo = nullptr;

			stageIndex++;
		}

		m_DescriptorSetLayout = shader->CreateAllDescriptorSetsLayout();
		m_PipelineLayout = Utils::CreatePipelineLayout(m_DescriptorSetLayout, shader->GetPushConstantSize());

		VkRayTracingPipelineCreateInfoKHR rayTracingPipelineInfo{};
		rayTracingPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		rayTracingPipelineInfo.stageCount = shaderStageCount;
		rayTracingPipelineInfo.pStages = shaderStages.data();
		rayTracingPipelineInfo.groupCount = (uint32_t)vulkanSBT->GetShaderGroups().size();
		rayTracingPipelineInfo.pGroups = vulkanSBT->GetShaderGroups().data();
		rayTracingPipelineInfo.maxPipelineRayRecursionDepth = 1;
		rayTracingPipelineInfo.layout = m_PipelineLayout;
		rayTracingPipelineInfo.basePipelineIndex = -1;
		rayTracingPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		VK_CHECK_RESULT(vkCreateRayTracingPipelinesKHR(
			device->GetVulkanDevice(),
			VK_NULL_HANDLE,
			VK_NULL_HANDLE,
			1,
			&rayTracingPipelineInfo,
			nullptr,
			&m_RayTracingPipeline),
			"Failed to Create Ray Tracing Pipeline");

		vulkanSBT->Invalidate(m_RayTracingPipeline);

		VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(), VK_OBJECT_TYPE_PIPELINE, m_DebugName, m_RayTracingPipeline);
	}

	void VulkanRayTracingPipeline::RT_InvalidateRayTracingPipeline()
	{
		Renderer::Submit([this]
		{
			auto device = VulkanContext::GetCurrentDevice();

			auto vulkanSBT = std::static_pointer_cast<VulkanShaderBindingTable>(m_ShaderBindingTable);
			auto shader = std::static_pointer_cast<VulkanRayTraceShader>(m_ShaderBindingTable->GetShader());
			auto& shaderSources = shader->GetShaderModules();

			for (auto&& [shaderPath, source] : shaderSources)
			{
				ShaderType stage = Utils::s_ShaderExtensionMap[shaderPath.extension()];

				switch (stage)
				{
				case ShaderType::RayGeneration:
					m_RayGenShaderModule = Utils::CreateShaderModule(source);
					break;
				case ShaderType::RayClosestHit:
				{
					VkShaderModule closestHitModule = Utils::CreateShaderModule(source);
					m_RayClosestHitShaderModules.push_back(closestHitModule);
					break;
				}
				case ShaderType::RayMiss:
				{
					VkShaderModule missModule = Utils::CreateShaderModule(source);
					m_RayMissShaderModules.push_back(missModule);
					break;
				}
				case ShaderType::RayIntersection:
				{
					VkShaderModule intersectionModule = Utils::CreateShaderModule(source);
					m_RayIntersectionShaderModules.push_back(intersectionModule);
					break;
				}
				default:
					break;
				}
			}

			size_t shaderStageCount = m_RayClosestHitShaderModules.size()
				+ m_RayMissShaderModules.size()
				+ m_RayIntersectionShaderModules.size() + 1;

			std::vector<VkPipelineShaderStageCreateInfo> shaderStages{ shaderStageCount };

			// Ray Gen
			shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			shaderStages[0].module = m_RayGenShaderModule;
			shaderStages[0].pName = "main";
			shaderStages[0].flags = 0;
			shaderStages[0].pNext = nullptr;
			shaderStages[0].pSpecializationInfo = nullptr;

			uint32_t stageIndex = 1;
			for (auto& rayClosestHitModule : m_RayClosestHitShaderModules)
			{
				shaderStages[stageIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStages[stageIndex].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
				shaderStages[stageIndex].module = rayClosestHitModule;
				shaderStages[stageIndex].pName = "main";
				shaderStages[stageIndex].flags = 0;
				shaderStages[stageIndex].pNext = nullptr;
				shaderStages[stageIndex].pSpecializationInfo = nullptr;

				stageIndex++;
			}

			for (auto& rayAnyHitModule : m_RayAnyHitShaderModules)
			{
				shaderStages[stageIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStages[stageIndex].stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
				shaderStages[stageIndex].module = rayAnyHitModule;
				shaderStages[stageIndex].pName = "main";
				shaderStages[stageIndex].flags = 0;
				shaderStages[stageIndex].pNext = nullptr;
				shaderStages[stageIndex].pSpecializationInfo = nullptr;

				stageIndex++;
			}

			for (auto& rayIntersectionModule : m_RayIntersectionShaderModules)
			{
				shaderStages[stageIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStages[stageIndex].stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
				shaderStages[stageIndex].module = rayIntersectionModule;
				shaderStages[stageIndex].pName = "main";
				shaderStages[stageIndex].flags = 0;
				shaderStages[stageIndex].pNext = nullptr;
				shaderStages[stageIndex].pSpecializationInfo = nullptr;

				stageIndex++;
			}

			for (auto& rayMissModule : m_RayMissShaderModules)
			{
				shaderStages[stageIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStages[stageIndex].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
				shaderStages[stageIndex].module = rayMissModule;
				shaderStages[stageIndex].pName = "main";
				shaderStages[stageIndex].flags = 0;
				shaderStages[stageIndex].pNext = nullptr;
				shaderStages[stageIndex].pSpecializationInfo = nullptr;

				stageIndex++;
			}

			m_DescriptorSetLayout = shader->CreateAllDescriptorSetsLayout();
			m_PipelineLayout = Utils::CreatePipelineLayout(m_DescriptorSetLayout, shader->GetPushConstantSize());

			VkRayTracingPipelineCreateInfoKHR rayTracingPipelineInfo{};
			rayTracingPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
			rayTracingPipelineInfo.stageCount = shaderStageCount;
			rayTracingPipelineInfo.pStages = shaderStages.data();
			rayTracingPipelineInfo.groupCount = (uint32_t)vulkanSBT->GetShaderGroups().size();
			rayTracingPipelineInfo.pGroups = vulkanSBT->GetShaderGroups().data();
			rayTracingPipelineInfo.maxPipelineRayRecursionDepth = 1;
			rayTracingPipelineInfo.layout = m_PipelineLayout;
			rayTracingPipelineInfo.basePipelineIndex = -1;
			rayTracingPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

			VK_CHECK_RESULT(vkCreateRayTracingPipelinesKHR(
				device->GetVulkanDevice(),
				VK_NULL_HANDLE,
				VK_NULL_HANDLE,
				1,
				&rayTracingPipelineInfo,
				nullptr,
				&m_RayTracingPipeline),
				"Failed to Create Ray Tracing Pipeline");

			vulkanSBT->Invalidate(m_RayTracingPipeline);

			VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(), VK_OBJECT_TYPE_PIPELINE, m_DebugName, m_RayTracingPipeline);
		});
	}

}
