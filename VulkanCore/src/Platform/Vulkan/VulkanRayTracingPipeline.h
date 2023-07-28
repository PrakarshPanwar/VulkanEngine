#pragma once
#include "VulkanContext.h"
#include "VulkanDescriptor.h"
#include "VulkanCore/Renderer/RayTracingPipeline.h"
#include "Platform/Vulkan/VulkanShader.h"

namespace VulkanCore {

	struct VulkanSBTInfo
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

		inline std::shared_ptr<Shader> GetShader() const override { return m_Shader; }
		inline VkPipelineLayout GetVulkanPipelineLayout() const { return m_PipelineLayout; }
		inline const VkStridedDeviceAddressRegionKHR& GetRayGenStridedDeviceAddressRegion() const { return m_RayGenSBTInfo.DeviceAddressRegion; }
		inline const VkStridedDeviceAddressRegionKHR& GetRayClosestHitStridedDeviceAddressRegion() const { return m_RayClosestHitSBTInfo.DeviceAddressRegion; }
		inline const VkStridedDeviceAddressRegionKHR& GetRayMissStridedDeviceAddressRegion() const { return m_RayMissSBTInfo.DeviceAddressRegion; }
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

		VulkanSBTInfo m_RayGenSBTInfo{}, m_RayClosestHitSBTInfo{}, m_RayMissSBTInfo{}, m_RayIntersectionSBTInfo{};
		std::shared_ptr<VulkanDescriptorSetLayout> m_DescriptorSetLayout;
	};

}
