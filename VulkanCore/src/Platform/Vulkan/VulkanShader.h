#pragma once
#include "VulkanCore/Renderer/Shader.h"
#include "VulkanDescriptor.h"

#include <future>

namespace VulkanCore {

	class VulkanShader : public Shader
	{
	public:
		VulkanShader() = default;
		VulkanShader(const std::string& vertexPath, const std::string& fragmentPath, const std::string& geometryPath = "");
		VulkanShader(const std::string& computePath);
		~VulkanShader();

		std::shared_ptr<VulkanDescriptorSetLayout> CreateDescriptorSetLayout(int index = 0);
		std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> CreateAllDescriptorSetsLayout();
		[[maybe_unused]] std::vector<VkDescriptorSet> AllocateDescriptorSets(uint32_t index = 0);
		[[maybe_unused]] std::vector<VkDescriptorSet> AllocateAllDescriptorSets();

		inline std::unordered_map<uint32_t, std::vector<uint32_t>>& GetShaderModules() { return m_VulkanSPIRV; }
		inline uint32_t GetPushConstantSize() const { return (uint32_t)m_PushConstantSize; }
		inline std::shared_ptr<VulkanDescriptorSetLayout> GetDescriptorSetLayout(uint32_t index = 0) const { return m_DescriptorSetsLayout[index]; }

		void Reload() override;
		inline bool HasGeometryShader() const override { return !m_GeometryFilePath.empty(); }
	private:
		std::tuple<std::string, std::string> ParseShader(const std::string& vertexPath, const std::string& fragmentPath);
		std::string ParseShader(const std::string& shaderPath);
		std::unordered_map<uint32_t, std::string> ParseShaders();
		void CompileOrGetVulkanBinaries(std::unordered_map<uint32_t, std::string>& shaderSources);
		void ReflectShaderData();
		void InvalidateDescriptors();
	protected:
		std::string ParsePreprocessIncludes(std::stringstream& sourceCode);
	private:
		std::unordered_map<uint32_t, std::vector<uint32_t>> m_VulkanSPIRV;
		std::string m_VertexFilePath, m_FragmentFilePath, m_GeometryFilePath, m_ComputeFilePath;
	protected:
		std::vector<std::future<void>> m_Futures;
		size_t m_PushConstantSize = 0;

		std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> m_DescriptorSetsLayout;
	};

}
