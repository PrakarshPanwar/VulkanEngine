#pragma once
#include "VulkanCore/Renderer/Shader.h"
#include "VulkanDescriptor.h"

#include <future>

namespace VulkanCore {

	class VulkanShader : public Shader
	{
	public:
		VulkanShader() = default;
		VulkanShader(const std::string& shaderName);
		~VulkanShader();

		std::shared_ptr<VulkanDescriptorSetLayout> CreateDescriptorSetLayout(int index = 0);
		std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> CreateAllDescriptorSetsLayout();
		[[maybe_unused]] std::vector<VkDescriptorSet> AllocateDescriptorSets(uint32_t index = 0);
		[[maybe_unused]] std::vector<VkDescriptorSet> AllocateAllDescriptorSets();

		inline std::map<uint32_t, std::vector<uint32_t>>& GetShaderModules() { return m_VulkanSPIRV; }
		inline uint32_t GetPushConstantSize() const { return (uint32_t)m_PushConstantSize; }
		inline std::shared_ptr<VulkanDescriptorSetLayout> GetDescriptorSetLayout(uint32_t index = 0) const { return m_DescriptorSetLayouts[index]; }

		void Reload() override;
		bool HasGeometryShader() const override;
		bool HasTessellationShaders() const override;
	private:
		void CheckRequiredStagesForCompilation();
		std::unordered_map<uint32_t, std::tuple<std::filesystem::path, std::string>> ParseShaders();
		std::string ParsePreprocessIncludes(std::stringstream& sourceCode);
		void CompileOrGetVulkanBinaries(std::unordered_map<uint32_t, std::tuple<std::filesystem::path, std::string>>& shaderSources);
		void ReflectShaderData();
		void InvalidateDescriptors();
	protected:
		std::string m_ShaderName;
		std::map<uint32_t, std::vector<uint32_t>> m_VulkanSPIRV;
		std::vector<std::future<void>> m_Futures;
		size_t m_PushConstantSize = 0;

		std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> m_DescriptorSetLayouts;
	};

}
