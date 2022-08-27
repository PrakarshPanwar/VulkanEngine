#pragma once
#include <future>

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

		inline std::unordered_map<uint32_t, std::vector<uint32_t>>& GetShaderModules() { return m_VulkanSPIRV; }
		inline bool CheckIfGeometryShaderExists() const { return m_HasGeometryShader; };
	private:
		std::tuple<std::string, std::string> ParseShader(const std::string& vsfilepath, const std::string& fsfilepath);
		std::tuple<std::string, std::string, std::string> ParseShader(const std::string& vsfilepath, const std::string& fsfilepath, const std::string& gsfilepath);
		void CompileOrGetVulkanBinaries(const std::unordered_map<uint32_t, std::string>& shaderSources);
		void ReflectShaderData();
	private:
		uint32_t m_RendererID;
		std::string m_VertexFilePath, m_FragmentFilePath, m_GeometryFilePath;
		std::unordered_map<uint32_t, std::string> m_ShaderSources;
		std::unordered_map<uint32_t, std::vector<uint32_t>> m_VulkanSPIRV;
		std::vector<std::future<void>> m_Futures;

		bool m_HasGeometryShader = false;
	};

}