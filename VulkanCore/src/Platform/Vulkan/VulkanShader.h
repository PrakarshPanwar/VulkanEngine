#pragma once
#include "VulkanCore/Renderer/Shader.h"
#include "VulkanDescriptor.h"

#include <future>

namespace VulkanCore {

	class VulkanShader : public Shader
	{
	public:
		VulkanShader() = default;
		VulkanShader(const std::string& vsfilepath, const std::string& fsfilepath, const std::string& gsfilepath = "");
		VulkanShader(const std::string& rtgenfilepath, const std::string& rthitfilepath, const std::string& rtmissfilepath, const std::string& rtintfilepath);
		VulkanShader(const std::string& cmpfilepath);
		~VulkanShader();

		std::shared_ptr<VulkanDescriptorSetLayout> CreateDescriptorSetLayout(int index = 0);
		std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> CreateAllDescriptorSetsLayout();
		std::vector<VkDescriptorSet> AllocateDescriptorSets(uint32_t index = 0);
		std::vector<VkDescriptorSet> AllocateAllDescriptorSets();

		inline std::unordered_map<uint32_t, std::vector<uint32_t>>& GetShaderModules() { return m_VulkanSPIRV; }
		inline uint32_t GetPushConstantSize() const { return (uint32_t)m_PushConstantSize; }
		inline std::shared_ptr<VulkanDescriptorSetLayout> GetDescriptorSetLayout(uint32_t index = 0) const { return m_DescriptorSetLayouts[index]; }

		void Reload() override;
		inline bool HasGeometryShader() const override { return !m_GeometryFilePath.empty(); }
		inline bool IsRayTraced() const override { return !m_RayTraceGenFilePath.empty(); }
		inline bool HasRayIntersectionShader() const override { return !m_RayTraceIntersectionFilePath.empty(); }
	private:
		std::tuple<std::string, std::string> ParseShader(const std::string& vsfilepath, const std::string& fsfilepath);
		std::tuple<std::string, std::string, std::string> ParseShader(const std::string& vsfilepath, const std::string& fsfilepath, const std::string& gsfilepath);
		std::tuple<std::string, std::string, std::string, std::string> ParseShader(const std::string & rtgenfilepath, const std::string & rthitfilepath, const std::string & rtmissfilepath, const std::string & rtintfilepath);
		std::string ParseShader(const std::string& cmpfilepath);
		void ParseShader();
		void CompileOrGetVulkanBinaries(const std::unordered_map<uint32_t, std::string>& shaderSources);
		void CompileOrGetVulkanRTBinaries(std::unordered_map<uint32_t, std::string>& shaderSources);
		void ReflectShaderData();
		void InvalidateDescriptors();
	private:
		std::string m_VertexFilePath, m_FragmentFilePath, m_GeometryFilePath, m_ComputeFilePath;
		std::string m_RayTraceGenFilePath, m_RayTraceHitFilePath, m_RayTraceMissFilePath, m_RayTraceIntersectionFilePath;
		std::unordered_map<uint32_t, std::string> m_ShaderSources;
		std::unordered_map<uint32_t, std::vector<uint32_t>> m_VulkanSPIRV;
		std::vector<std::future<void>> m_Futures;
		size_t m_PushConstantSize = 0;

		std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> m_DescriptorSetLayouts;
	};

}
