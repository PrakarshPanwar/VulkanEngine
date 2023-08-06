#pragma once
#include "VulkanShader.h"
#include "VulkanDescriptor.h"

#include <future>

namespace VulkanCore {

	class VulkanRayTraceShader : public VulkanShader
	{
	public:
		VulkanRayTraceShader() = default;
		VulkanRayTraceShader(const std::string& rayGenPath, const std::string& rayClosestHitPath, const std::string& rayMissPath);
		VulkanRayTraceShader(const std::string& rayGenPath, const std::string& rayClosestHitPath, const std::string& rayAnyHitPath, const std::string& rayIntersectionPath, const std::string& rayMissPath);
		~VulkanRayTraceShader();

		std::shared_ptr<VulkanDescriptorSetLayout> CreateDescriptorSetLayout(int index = 0);
		std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> CreateAllDescriptorSetsLayout();

		void Reload() override;
		inline bool HasAnyHitShader() const { return !m_RayAnyHitFilePath.empty(); }
		inline bool HasIntersectionShader() const { return !m_RayIntersectionFilePath.empty(); }
	private:
		std::tuple<std::string, std::string, std::string> ParseShader(const std::string& rayGenPath, const std::string& rayClosestHitPath, const std::string& rayMissPath);
		void ParseShader();
		void CompileOrGetVulkanBinaries(const std::unordered_map<uint32_t, std::string>& shaderSources);
		void ReflectShaderData();
	private:
		std::string m_RayGenFilePath, m_RayClosestHitFilePath, m_RayAnyHitFilePath, m_RayIntersectionFilePath, m_RayMissFilePath;
	};

}
