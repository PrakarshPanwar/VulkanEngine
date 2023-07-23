#pragma once
#include "VulkanContext.h"
#include "VulkanDescriptor.h"
#include "VulkanCore/Renderer/RayTracingPipeline.h"
#include "Platform/Vulkan/VulkanShader.h"

namespace VulkanCore {

	struct VulkanShaderBindingInfo
	{
		VkBuffer Buffer = nullptr;
		VmaAllocation MemoryAlloc = nullptr;
		VkStridedDeviceAddressRegionKHR DeviceAddressRegion{};
	};

	class VulkanRayTracingPipeline : public RayTracingPipeline
	{
	public:
		VulkanRayTracingPipeline() = default;
		VulkanRayTracingPipeline(std::shared_ptr<Shader> shader, const std::string& debugName);
		~VulkanRayTracingPipeline();

		void Release();

		void Bind(VkCommandBuffer commandBuffer);
		void CreateShaderBindingTable() override;

		inline VkPipelineLayout GetVulkanPipelineLayout() const { return m_PipelineLayout; }
	private:
		void InvalidateRayTracingPipeline();
		void RT_InvalidateRayTracingPipeline();
	private:
		std::string m_DebugName;

		VkPipeline m_RayTracingPipeline = nullptr;
		VkPipelineLayout m_PipelineLayout = nullptr;
		VkShaderModule m_RayGenShaderModule, m_RayClosestHitShaderModule, m_RayMissShaderModule, m_RayIntersectionShaderModule = nullptr;
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_ShaderGroups{};
		std::shared_ptr<Shader> m_Shader;

		VulkanShaderBindingInfo m_RayGenBindingInfo, m_RayClosestHitBindingInfo, m_RayMissBindingInfo, m_RayIntersectionBindingInfo{};
		std::shared_ptr<VulkanDescriptorSetLayout> m_DescriptorSetLayout;
	};

}
