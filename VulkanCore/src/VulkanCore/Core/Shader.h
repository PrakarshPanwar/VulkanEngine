#pragma once
#include <future>
#include "Platform/Vulkan/VulkanDescriptor.h"
#include "../Source/SPIRV-Reflect/spirv_reflect.h"

namespace VulkanCore {

	enum class ShaderType
	{
		Vertex = 0x8B31,
		Fragment = 0x8B30,
		Geometry = 0x8DD9
	};

	class Shader
	{
	public:
		Shader() = default;

		Shader(const std::string& vsfilepath, const std::string& fsfilepath, const std::string& gsfilepath = "");
		~Shader();

		std::shared_ptr<VulkanDescriptorSetLayout> CreateDescriptorSetLayout();

		inline std::unordered_map<uint32_t, std::vector<uint32_t>>& GetShaderModules() { return m_VulkanSPIRV; }
		inline bool CheckIfGeometryShaderExists() const { return m_HasGeometryShader; };
	private:
		std::tuple<std::string, std::string> ParseShader(const std::string& vsfilepath, const std::string& fsfilepath);
		std::tuple<std::string, std::string, std::string> ParseShader(const std::string& vsfilepath, const std::string& fsfilepath, const std::string& gsfilepath);
		void CompileOrGetVulkanBinaries(const std::unordered_map<uint32_t, std::string>& shaderSources);
		void ReflectShaderData();
	private:
		std::string m_VertexFilePath, m_FragmentFilePath, m_GeometryFilePath;
		std::unordered_map<uint32_t, std::string> m_ShaderSources;
		std::unordered_map<uint32_t, std::vector<uint32_t>> m_VulkanSPIRV;
		std::vector<std::future<void>> m_Futures;
		std::vector<VkDescriptorSet> m_DescriptorSet;

		bool m_HasGeometryShader = false;
	};

}