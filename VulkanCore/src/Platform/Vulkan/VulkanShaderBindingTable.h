#pragma once
#include "VulkanContext.h"
#include "VulkanCore/Renderer/ShaderBindingTable.h"

namespace VulkanCore {

	struct HitShaderInfo
	{
		std::string ClosestHitPath{};
		std::string AnyHitPath{};
		std::string IntersectionPath{};
	};

	class VulkanShaderBindingTable : public ShaderBindingTable
	{
	public:
		VulkanShaderBindingTable(const std::string& rayGenPath, const std::vector<HitShaderInfo>& hitShaderInfos, const std::vector<std::string>& rayMissPaths);
		~VulkanShaderBindingTable();

		// NOTE: Should run inside RayTracingPipeline
		void Invalidate(VkPipeline pipeline);

		inline std::shared_ptr<Shader> GetShader() const override { return m_Shader; }

		inline const std::vector<VkRayTracingShaderGroupCreateInfoKHR>& GetShaderGroups() const { return m_ShaderGroups; }
		inline const VkStridedDeviceAddressRegionKHR& GetRayGenStridedDeviceAddressRegion() const { return m_RayGenSBTInfo; }
		inline const VkStridedDeviceAddressRegionKHR& GetRayHitStridedDeviceAddressRegion() const { return m_RayClosestHitSBTInfo; }
		inline const VkStridedDeviceAddressRegionKHR& GetRayMissStridedDeviceAddressRegion() const { return m_RayMissSBTInfo; }
	private:
		void InvalidateShader();
	private:
		std::shared_ptr<Shader> m_Shader;

		std::string m_RayGenPath;
		std::vector<HitShaderInfo> m_HitShaderInfos;
		std::vector<std::string> m_RayMissPaths;

		VkBuffer m_SBTBuffer = nullptr;
		VmaAllocation m_SBTMemAlloc = nullptr;
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_ShaderGroups;
		VkStridedDeviceAddressRegionKHR m_RayGenSBTInfo{}, m_RayClosestHitSBTInfo{}, m_RayMissSBTInfo{}, m_RayIntersectionSBTInfo{};
	};

}
