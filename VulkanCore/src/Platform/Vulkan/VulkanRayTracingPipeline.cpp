#include "vulkanpch.h"
#include "VulkanRayTracingPipeline.h"
#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "VulkanAllocator.h"

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

		static uint32_t GetAlignment(uint32_t size, uint32_t alignment)
		{
			return (size + alignment - 1) & ~(alignment - 1);
		}

		static VkStridedDeviceAddressRegionKHR GetSBTStridedDeviceAddressRegion(VkBuffer buffer, uint32_t offset = 0, uint32_t handleCount = 1)
		{
			auto device = VulkanContext::GetCurrentDevice();

			auto rayTracingPipelineProperties = device->GetPhysicalDeviceRayTracingPipelineProperties();
			const uint32_t shaderGroupBaseAlignment = rayTracingPipelineProperties.shaderGroupBaseAlignment;
			const uint32_t shaderGroupHandleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
			const uint32_t shaderHandleSizeAligned = GetAlignment(shaderGroupHandleSize * handleCount, shaderGroupBaseAlignment);

			VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
			bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			bufferDeviceAddressInfo.buffer = buffer;

			VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegionKHR{};
			stridedDeviceAddressRegionKHR.deviceAddress = vkGetBufferDeviceAddressKHR(device->GetVulkanDevice(), &bufferDeviceAddressInfo) + (VkDeviceAddress)offset;
			stridedDeviceAddressRegionKHR.stride = shaderHandleSizeAligned;
			stridedDeviceAddressRegionKHR.size = shaderHandleSizeAligned;
			return stridedDeviceAddressRegionKHR;
		}

	}

	VulkanRayTracingPipeline::VulkanRayTracingPipeline(std::shared_ptr<Shader> shader, const std::string& debugName)
		: m_DebugName(debugName), m_Shader(shader)
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
		VulkanAllocator allocator("ShaderBindingTable");

		// Destroy Pipeline
		vkDestroyShaderModule(device->GetVulkanDevice(), m_RayGenShaderModule, nullptr);
		vkDestroyShaderModule(device->GetVulkanDevice(), m_RayClosestHitShaderModule, nullptr);
		vkDestroyShaderModule(device->GetVulkanDevice(), m_RayMissShaderModule, nullptr);

		if (m_RayIntersectionShaderModule)
			vkDestroyShaderModule(device->GetVulkanDevice(), m_RayIntersectionShaderModule, nullptr);

		if (m_PipelineLayout)
			vkDestroyPipelineLayout(device->GetVulkanDevice(), m_PipelineLayout, nullptr);

		vkDestroyPipeline(device->GetVulkanDevice(), m_RayTracingPipeline, nullptr);

		m_RayTracingPipeline = nullptr;

		// Destroy SBT
		allocator.DestroyBuffer(m_SBTBuffer, m_SBTMemAlloc);
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

	void VulkanRayTracingPipeline::ReloadPipeline()
	{
		if (m_Shader->GetReloadFlag())
		{
			Release();
			InvalidateRayTracingPipeline();
			m_Shader->ResetReloadFlag();
		}
	}

	void VulkanRayTracingPipeline::InvalidateRayTracingPipeline()
	{
		auto device = VulkanContext::GetCurrentDevice();

		auto shader = std::static_pointer_cast<VulkanRayTraceShader>(m_Shader);
		auto& shaderSources = shader->GetShaderModules();

		m_RayGenShaderModule = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::RayGeneration]);
		m_RayClosestHitShaderModule = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::RayClosestHit]);
		m_RayMissShaderModule = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::RayMiss]);

		const uint32_t shaderStageCount = shader->HasIntersectionShader() ? 4 : 3;
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages(shaderStageCount);

		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		shaderStages[0].module = m_RayGenShaderModule;
		shaderStages[0].pName = "main";
		shaderStages[0].flags = 0;
		shaderStages[0].pNext = nullptr;
		shaderStages[0].pSpecializationInfo = nullptr;

		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		shaderStages[1].module = m_RayClosestHitShaderModule;
		shaderStages[1].pName = "main";
		shaderStages[1].flags = 0;
		shaderStages[1].pNext = nullptr;
		shaderStages[1].pSpecializationInfo = nullptr;

		shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[2].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		shaderStages[2].module = m_RayMissShaderModule;
		shaderStages[2].pName = "main";
		shaderStages[2].flags = 0;
		shaderStages[2].pNext = nullptr;
		shaderStages[2].pSpecializationInfo = nullptr;

		if (shader->HasIntersectionShader())
		{
			m_RayIntersectionShaderModule = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::RayIntersection]);

			shaderStages[3].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[3].stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
			shaderStages[3].module = m_RayClosestHitShaderModule;
			shaderStages[3].pName = "main";
			shaderStages[3].flags = 0;
			shaderStages[3].pNext = nullptr;
			shaderStages[3].pSpecializationInfo = nullptr;
		}

		// Set Shader Groups
		uint32_t shaderGroupIndex = 0;

		// Ray Generation
		VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{};
		shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroupInfo.generalShader = shaderGroupIndex++;
		shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
		m_ShaderGroups.push_back(shaderGroupInfo);

		// Closest Hit
		shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		shaderGroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
		shaderGroupInfo.closestHitShader = shaderGroupIndex++;
		m_ShaderGroups.push_back(shaderGroupInfo);

		// Miss
		shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroupInfo.generalShader = shaderGroupIndex++;
		shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		m_ShaderGroups.push_back(shaderGroupInfo);

		m_DescriptorSetLayout = shader->CreateAllDescriptorSetsLayout();
		m_PipelineLayout = Utils::CreatePipelineLayout(m_DescriptorSetLayout, shader->GetPushConstantSize());

		VkRayTracingPipelineCreateInfoKHR rayTracingPipelineInfo{};
		rayTracingPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		rayTracingPipelineInfo.stageCount = shaderStageCount;
		rayTracingPipelineInfo.pStages = shaderStages.data();
		rayTracingPipelineInfo.groupCount = (uint32_t)m_ShaderGroups.size();
		rayTracingPipelineInfo.pGroups = m_ShaderGroups.data();
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

		VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(), VK_OBJECT_TYPE_PIPELINE, m_DebugName, m_RayTracingPipeline);
	}

	void VulkanRayTracingPipeline::RT_InvalidateRayTracingPipeline()
	{
		Renderer::Submit([this]
		{
			auto device = VulkanContext::GetCurrentDevice();

			auto shader = std::static_pointer_cast<VulkanRayTraceShader>(m_Shader);
			auto& shaderSources = shader->GetShaderModules();

			m_RayGenShaderModule = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::RayGeneration]);
			m_RayClosestHitShaderModule = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::RayClosestHit]);
			m_RayMissShaderModule = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::RayMiss]);

			const uint32_t shaderStageCount = shader->HasIntersectionShader() ? 4 : 3;
			std::vector<VkPipelineShaderStageCreateInfo> shaderStages(shaderStageCount);

			shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			shaderStages[0].module = m_RayGenShaderModule;
			shaderStages[0].pName = "main";
			shaderStages[0].flags = 0;
			shaderStages[0].pNext = nullptr;
			shaderStages[0].pSpecializationInfo = nullptr;

			shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[1].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			shaderStages[1].module = m_RayClosestHitShaderModule;
			shaderStages[1].pName = "main";
			shaderStages[1].flags = 0;
			shaderStages[1].pNext = nullptr;
			shaderStages[1].pSpecializationInfo = nullptr;

			shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[2].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
			shaderStages[2].module = m_RayMissShaderModule;
			shaderStages[2].pName = "main";
			shaderStages[2].flags = 0;
			shaderStages[2].pNext = nullptr;
			shaderStages[2].pSpecializationInfo = nullptr;

			if (shader->HasIntersectionShader())
			{
				m_RayIntersectionShaderModule = Utils::CreateShaderModule(shaderSources[(uint32_t)ShaderType::RayIntersection]);

				shaderStages[3].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStages[3].stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
				shaderStages[3].module = m_RayClosestHitShaderModule;
				shaderStages[3].pName = "main";
				shaderStages[3].flags = 0;
				shaderStages[3].pNext = nullptr;
				shaderStages[3].pSpecializationInfo = nullptr;
			}

			// Set Shader Groups
			// Ray Generation
			VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{};
			shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroupInfo.generalShader = 0;
			shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
			m_ShaderGroups.push_back(shaderGroupInfo);

			// Closest Hit
			shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			shaderGroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
			shaderGroupInfo.closestHitShader = 1;
			m_ShaderGroups.push_back(shaderGroupInfo);

			// Miss
			shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroupInfo.generalShader = 2;
			shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
			m_ShaderGroups.push_back(shaderGroupInfo);

			m_DescriptorSetLayout = shader->CreateAllDescriptorSetsLayout();
			m_PipelineLayout = Utils::CreatePipelineLayout(m_DescriptorSetLayout, shader->GetPushConstantSize());

			VkRayTracingPipelineCreateInfoKHR rayTracingPipelineInfo{};
			rayTracingPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
			rayTracingPipelineInfo.stageCount = shaderStageCount;
			rayTracingPipelineInfo.pStages = shaderStages.data();
			rayTracingPipelineInfo.groupCount = (uint32_t)m_ShaderGroups.size();
			rayTracingPipelineInfo.pGroups = m_ShaderGroups.data();
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

			VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(), VK_OBJECT_TYPE_PIPELINE, m_DebugName, m_RayTracingPipeline);
		});
	}

}
