#pragma once
#include <future>
#include "Platform/Vulkan/VulkanDescriptor.h"
#include "../Source/SPIRV-Reflect/spirv_reflect.h"

#define USE_VULKAN_DESCRIPTOR 0

namespace VulkanCore {

	enum class ShaderType
	{
		Vertex = 0x8B31,
		Fragment = 0x8B30,
		Geometry = 0x8DD9,
		Compute = 0x91B9
	};

	class Shader
	{
	public:
		Shader() = default;

		Shader(const std::string& vsfilepath, const std::string& fsfilepath, const std::string& gsfilepath = "");
		Shader(const std::string& cmpfilepath);
		~Shader();

#if USE_VULKAN_DESCRIPTOR
		std::shared_ptr<VulkanDescriptorSetLayout> CreateDescriptorSetLayout(int index = 0);
		std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> CreateAllDescriptorSetsLayout();
#endif
		std::vector<VkDescriptorSet> AllocateDescriptorSets(uint32_t index = 0);
		std::vector<VkDescriptorSet> AllocateAllDescriptorSets();

		inline std::unordered_map<uint32_t, std::vector<uint32_t>>& GetShaderModules() { return m_VulkanSPIRV; }
		inline uint32_t GetPushConstantSize() const { return (uint32_t)m_PushConstantSize; }

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

		std::unordered_map<uint32_t, VkWriteDescriptorSet> m_WriteDescriptors;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_DescriptorLayoutBindings;
		bool m_HasGeometryShader = false;
	};

}