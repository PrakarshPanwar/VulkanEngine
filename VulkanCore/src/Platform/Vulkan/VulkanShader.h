#pragma once
#include "VulkanCore/Renderer/Shader.h"

namespace VulkanCore {

	class VulkanShader : public Shader
	{
		VulkanShader(const std::string& vsfilepath, const std::string& fsfilepath, const std::string& gsfilepath = "");
		VulkanShader(const std::string& cmpfilepath);
		~VulkanShader();

#if USE_VULKAN_DESCRIPTOR
		std::shared_ptr<VulkanDescriptorSetLayout> CreateDescriptorSetLayout(int index = 0);
		std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> CreateAllDescriptorSetsLayout();

		VkDescriptorSetLayout CreateVulkanDescriptorSetLayout(uint32_t index = 0);
#endif
		std::vector<VkDescriptorSet> AllocateDescriptorSets(uint32_t index = 0);
		std::vector<VkDescriptorSet> AllocateAllDescriptorSets();

		inline std::unordered_map<uint32_t, std::vector<uint32_t>>& GetShaderModules() { return m_VulkanSPIRV; }
		inline uint32_t GetPushConstantSize() const { return (uint32_t)m_PushConstantSize; }
		inline std::shared_ptr<VulkanDescriptorSetLayout> GetDescriptorSetLayout(uint32_t index = 0) const { return m_DescriptorSetLayouts[index]; }

		inline bool CheckIfGeometryShaderExists() const { return m_HasGeometryShader; };
	private:
		std::tuple<std::string, std::string> ParseShader(const std::string& vsfilepath, const std::string& fsfilepath);
		std::tuple<std::string, std::string, std::string> ParseShader(const std::string& vsfilepath, const std::string& fsfilepath, const std::string& gsfilepath);
		std::string ParseShader(const std::string& cmpfilepath);
		void CompileOrGetVulkanBinaries(const std::unordered_map<uint32_t, std::string>& shaderSources);
		void ReflectShaderData();
		void InvalidateDescriptors();
	private:
		std::string m_VertexFilePath, m_FragmentFilePath, m_GeometryFilePath, m_ComputeFilePath;
		std::unordered_map<uint32_t, std::string> m_ShaderSources;
		std::unordered_map<uint32_t, std::vector<uint32_t>> m_VulkanSPIRV;
		std::vector<std::future<void>> m_Futures;
		size_t m_PushConstantSize = 0;

		std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> m_DescriptorSetLayouts;
		bool m_HasGeometryShader = false;
	};

}