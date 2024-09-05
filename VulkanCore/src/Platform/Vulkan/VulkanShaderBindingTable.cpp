#include "vulkanpch.h"
#include "VulkanShaderBindingTable.h"
#include "VulkanRayTraceShader.h"
#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "VulkanAllocator.h"

#include <algorithm>

namespace VulkanCore {

	namespace Utils {

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
			stridedDeviceAddressRegionKHR.stride = shaderGroupHandleSize;
			stridedDeviceAddressRegionKHR.size = shaderHandleSizeAligned;
			return stridedDeviceAddressRegionKHR;
		}

		static VkRayTracingShaderGroupTypeKHR VulkanShaderGroupType(const HitShaderInfo& shaderInfo)
		{
			return shaderInfo.AnyHitPath.empty() || shaderInfo.IntersectionPath.empty() ?
				VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR
				: VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		}

		static uint32_t GetHitGroupShaderIndex(const std::vector<std::string>& hitShaderGroup, const std::vector<std::string>::iterator& InputIter)
		{
			uint32_t index = std::ranges::distance(hitShaderGroup.begin(), InputIter);
			return index >= hitShaderGroup.size() ? VK_SHADER_UNUSED_KHR : index + 1;
		}

	}

	static std::filesystem::path g_ShaderPath = "assets\\shaders";

	VulkanShaderBindingTable::VulkanShaderBindingTable(const std::string& rayGenPath, const std::vector<HitShaderInfo>& hitShaderInfos, const std::vector<std::string>& rayMissPaths)
		: m_RayGenPath(rayGenPath), m_HitShaderInfos(hitShaderInfos), m_RayMissPaths(rayMissPaths)
	{
		InvalidateShader();
	}

	VulkanShaderBindingTable::~VulkanShaderBindingTable()
	{
		if (m_SBTBuffer)
			Release();
	}

	void VulkanShaderBindingTable::Invalidate(VkPipeline pipeline)
	{
		if (m_SBTBuffer)
			Release();

		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("ShaderBindingTable");

		auto rayTracingPipelineProperties = device->GetPhysicalDeviceRayTracingPipelineProperties();
		const uint32_t shaderGroupBaseAlignment = rayTracingPipelineProperties.shaderGroupBaseAlignment;
		const uint32_t shaderGroupHandleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
		const uint32_t shaderGroupHandleAlignment = rayTracingPipelineProperties.shaderGroupHandleAlignment;
		const uint32_t shaderHandleSizeAligned = Utils::GetAlignment(shaderGroupHandleSize, shaderGroupHandleAlignment);
		const uint32_t groupCount = (uint32_t)(m_HitShaderInfos.size() + m_RayMissPaths.size() + 1);
		const uint32_t sbtSize = groupCount * shaderHandleSizeAligned;

		// Get Shader Handles
		std::vector<uint8_t> shaderHandleStorage(sbtSize);
		VK_CHECK_RESULT(vkGetRayTracingShaderGroupHandlesKHR(
			device->GetVulkanDevice(),
			pipeline,
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

		// Ray Generation
		m_RayGenSBTInfo = Utils::GetSBTStridedDeviceAddressRegion(m_SBTBuffer);
		memcpy(sbtBufferDst, shaderHandleStorage.data(), shaderHandleSizeAligned);
		sbtBufferDst += m_RayGenSBTInfo.size;
		offset += m_RayGenSBTInfo.size;
		m_RayGenSBTInfo.stride = m_RayGenSBTInfo.size;

		// Hit
		m_RayHitSBTInfo = Utils::GetSBTStridedDeviceAddressRegion(m_SBTBuffer, offset, (uint32_t)m_HitShaderInfos.size());
		memcpy(sbtBufferDst, shaderHandleStorage.data() + shaderHandleSizeAligned, shaderGroupHandleSize * m_HitShaderInfos.size());
		sbtBufferDst += m_RayHitSBTInfo.size;
		offset += m_RayHitSBTInfo.size;

		// Miss
		m_RayMissSBTInfo = Utils::GetSBTStridedDeviceAddressRegion(m_SBTBuffer, offset, (uint32_t)m_RayMissPaths.size());
		memcpy(sbtBufferDst, shaderHandleStorage.data() + shaderHandleSizeAligned * (m_HitShaderInfos.size() + 1), shaderGroupHandleSize * m_RayMissPaths.size());

		allocator.UnmapMemory(m_SBTMemAlloc);
	}

	void VulkanShaderBindingTable::Release()
	{
		Renderer::SubmitResourceFree([sbtBuffer = m_SBTBuffer, sbtMemAlloc = m_SBTMemAlloc]() mutable
		{
			VulkanAllocator allocator("ShaderBindingTable");
			allocator.DestroyBuffer(sbtBuffer, sbtMemAlloc);
		});

		m_SBTBuffer = nullptr;
		m_SBTMemAlloc = nullptr;
	}

	void VulkanShaderBindingTable::InvalidateShader()
	{
		// Create Shader
		{
			// Ray Generation
			std::filesystem::path rayGenAbsPath = g_ShaderPath / m_RayGenPath;

			// Hit Groups
			std::vector<std::string> rayClosestHitAbsPaths, rayAnyHitAbsPaths, rayIntersectionAbsPaths, rayMissAbsPaths;
			for (auto& hitShaderInfo : m_HitShaderInfos)
			{
				// Closest Hit
				if (!hitShaderInfo.ClosestHitPath.empty())
				{
					std::filesystem::path rayClosestHitAbsPath = g_ShaderPath / hitShaderInfo.ClosestHitPath;
					rayClosestHitAbsPaths.emplace_back(rayClosestHitAbsPath.string());
				}

				// Any Hit
				if (!hitShaderInfo.AnyHitPath.empty())
				{
					std::filesystem::path rayAnyHitAbsPath = g_ShaderPath / hitShaderInfo.AnyHitPath;
					rayAnyHitAbsPaths.emplace_back(rayAnyHitAbsPath.string());
				}

				// Intersection
				if (!hitShaderInfo.IntersectionPath.empty())
				{
					std::filesystem::path rayIntersectionAbsPath = g_ShaderPath / hitShaderInfo.IntersectionPath;
					rayIntersectionAbsPaths.emplace_back(rayIntersectionAbsPath.string());
				}
			}

			// Miss Groups
			for (auto& missPath : m_RayMissPaths)
			{
				std::filesystem::path rayMissAbsPath = g_ShaderPath / missPath;
				rayMissAbsPaths.emplace_back(rayMissAbsPath.string());
			}

			m_Shader = std::make_shared<VulkanRayTraceShader>(rayGenAbsPath.string(),
				rayClosestHitAbsPaths, rayAnyHitAbsPaths, rayIntersectionAbsPaths,
				rayMissAbsPaths);
		}

		// Sort Shader Groups
		{
			std::vector<std::string> closestHitPaths, anyHitPaths, intersectionPaths;
			for (auto& hitShaderInfo : m_HitShaderInfos)
			{
				closestHitPaths.push_back(hitShaderInfo.ClosestHitPath);
				anyHitPaths.push_back(hitShaderInfo.AnyHitPath);
				intersectionPaths.push_back(hitShaderInfo.IntersectionPath);
			}

			// Remove Empty Shader Strings
			auto RemoveEmpty = [](const std::string& s)
			{
				return s.empty();
			};

			auto RemoveIt1 = std::ranges::remove_if(closestHitPaths, RemoveEmpty);
			auto RemoveIt2 = std::ranges::remove_if(anyHitPaths, RemoveEmpty);
			auto RemoveIt3 = std::ranges::remove_if(intersectionPaths, RemoveEmpty);

			closestHitPaths.erase(RemoveIt1.begin(), RemoveIt1.end());
			anyHitPaths.erase(RemoveIt2.begin(), RemoveIt2.end());
			intersectionPaths.erase(RemoveIt3.begin(), RemoveIt3.end());

			// Ray Generation
 			VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{};
			shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroupInfo.generalShader = 0;
			shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
			m_ShaderGroups.push_back(shaderGroupInfo);

			shaderGroupInfo.generalShader = VK_SHADER_UNUSED_KHR;

			std::vector<std::string> allHitShaders{};
			allHitShaders.insert(allHitShaders.end(), closestHitPaths.begin(), closestHitPaths.end());
			allHitShaders.insert(allHitShaders.end(), anyHitPaths.begin(), anyHitPaths.end());
			allHitShaders.insert(allHitShaders.end(), intersectionPaths.begin(), intersectionPaths.end());

			// Hit Shaders
			for (auto& hitShaderInfo : m_HitShaderInfos)
			{
				shaderGroupInfo.type = Utils::VulkanShaderGroupType(hitShaderInfo);

				// Closest Hit
				const auto OutputCHShader = std::ranges::find(allHitShaders.begin(), allHitShaders.end(), hitShaderInfo.ClosestHitPath);
				shaderGroupInfo.closestHitShader = Utils::GetHitGroupShaderIndex(allHitShaders, OutputCHShader);

				// Any Hit
				const auto OutputAHShader = std::ranges::find(allHitShaders.begin(), allHitShaders.end(), hitShaderInfo.AnyHitPath);
				shaderGroupInfo.anyHitShader = Utils::GetHitGroupShaderIndex(allHitShaders, OutputAHShader);

				// Intersection
				const auto OutputIntShader = std::ranges::find(allHitShaders.begin(), allHitShaders.end(), hitShaderInfo.IntersectionPath);
				shaderGroupInfo.intersectionShader = Utils::GetHitGroupShaderIndex(allHitShaders, OutputIntShader);

				m_ShaderGroups.push_back(shaderGroupInfo);
			}

			// Miss Shaders
			shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

			uint32_t index = (uint32_t)allHitShaders.size() + 1;
			for (auto& missPath : m_RayMissPaths)
			{
				shaderGroupInfo.generalShader = index++;
				m_ShaderGroups.push_back(shaderGroupInfo);
			}
		}

	}

}
