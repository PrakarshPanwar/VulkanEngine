#pragma once
#include "VulkanCore/Renderer/Shader.h"
#include "VulkanDescriptor.h"

#include <future>

namespace VulkanCore {

	class VulkanShader : public Shader
	{
	public:
		VulkanShader() = default;
		VulkanShader(const std::string& vertexPath, const std::string& fragmentPath, const std::string& geometryPath = "",
			const std::string& tessellationControlPath = "", const std::string& tessellationEvaluationPath = "");

		VulkanShader(const std::string& computePath);
		~VulkanShader();

		std::shared_ptr<VulkanDescriptorSetLayout> CreateDescriptorSetLayout(int index = 0);
		std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> CreateAllDescriptorSetsLayout();
		[[maybe_unused]] std::vector<VkDescriptorSet> AllocateDescriptorSets(uint32_t index = 0);
		[[maybe_unused]] std::vector<VkDescriptorSet> AllocateAllDescriptorSets();

		inline std::unordered_map<uint32_t, std::vector<uint32_t>>& GetShaderModules() { return m_VulkanSPIRV; }
		inline uint32_t GetPushConstantSize() const { return (uint32_t)m_PushConstantSize; }
		inline std::shared_ptr<VulkanDescriptorSetLayout> GetDescriptorSetLayout(uint32_t index = 0) const { return m_DescriptorSetLayouts[index]; }

		void Reload() override;
		inline bool HasGeometryShader() const override { return !m_GeometryFilePath.empty(); }
		inline bool HasTessellationShaders() const override { return !(m_TessellationControlFilePath.empty() || m_TessellationEvaluationFilePath.empty()); }
	private:
		std::tuple<std::string, std::string> ParseShader(const std::string& vertexPath, const std::string& fragmentPath);
		std::string ParseShader(const std::string& shaderPath);
		std::unordered_map<uint32_t, std::string> ParseShaders();
		std::string ParsePreprocessIncludes(std::stringstream& sourceCode);
		void CompileOrGetVulkanBinaries(std::unordered_map<uint32_t, std::string>& shaderSources);
		void ReflectShaderData();
		void InvalidateDescriptors();
	private:
		std::string m_VertexFilePath, m_FragmentFilePath, m_GeometryFilePath,
			m_TessellationControlFilePath, m_TessellationEvaluationFilePath, m_ComputeFilePath;

		std::unordered_map<uint32_t, std::vector<uint32_t>> m_VulkanSPIRV;
		std::vector<std::future<void>> m_Futures;
		size_t m_PushConstantSize = 0;

		std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> m_DescriptorSetLayouts;
	};

}
