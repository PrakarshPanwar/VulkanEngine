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
		VulkanRayTraceShader(const std::string& rayGenPath, const std::vector<std::string>& rayClosestHitPaths, const std::vector<std::string>& rayAnyHitPaths, const std::vector<std::string>& rayIntersectionPaths, const std::vector<std::string>& rayMissPaths);
		~VulkanRayTraceShader();

		std::shared_ptr<VulkanDescriptorSetLayout> CreateDescriptorSetLayout(int index = 0);
		std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> CreateAllDescriptorSetsLayout();

		inline std::unordered_map<std::filesystem::path, std::vector<uint32_t>>& GetShaderModules() { return m_VulkanSPIRV; }

		void Reload() override;
		inline bool HasAnyHitShader() const { return !m_RayAnyHitFilePaths.empty(); }
		inline bool HasIntersectionShader() const { return !m_RayIntersectionFilePaths.empty(); }
	private:
		std::tuple<std::string, std::string, std::string> ParseShader(const std::string& rayGenPath, const std::string& rayClosestHitPath, const std::string& rayMissPath);
		void ParseShader();
		void CompileOrGetVulkanBinaries(const std::unordered_map<std::filesystem::path, std::string>& shaderSources);
		void ReflectShaderData();
	private:
		std::unordered_map<std::filesystem::path, std::string> m_ShaderSources;
		std::unordered_map<std::filesystem::path, std::vector<uint32_t>> m_VulkanSPIRV;

		std::string m_RayGenFilePath;
		std::vector<std::string> m_RayClosestHitFilePaths, m_RayAnyHitFilePaths, m_RayIntersectionFilePaths, m_RayMissFilePaths;
	};

}
