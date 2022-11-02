#include "vulkanpch.h"
#include "Shader.h"

#include "Platform/Vulkan/VulkanSwapChain.h"
#include "Application.h"
#include "Assert.h"
#include "Log.h"
#include "Timer.h"

#include <filesystem>
#include <mutex>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include "Platform/Vulkan/VulkanDescriptor.h"

namespace VulkanCore {

	namespace Utils {

		static shaderc_shader_kind GLShaderStageToShaderC(ShaderType stage)
		{
			switch (stage)
			{
			case ShaderType::Vertex:   return shaderc_glsl_vertex_shader;
			case ShaderType::Fragment: return shaderc_glsl_fragment_shader;
			case ShaderType::Geometry: return shaderc_glsl_geometry_shader;
			}

			VK_CORE_ASSERT(false, "Cannot find Shader Type!");
			return (shaderc_shader_kind)0;
		}

		static std::string GLShaderTypeToString(ShaderType stage)
		{
			switch (stage)
			{
			case ShaderType::Vertex:   return "Vertex";
			case ShaderType::Fragment: return "Fragment";
			case ShaderType::Geometry: return "Geometry";
			}

			VK_CORE_ASSERT(false, "Cannot find Shader Type!");
			return {};
		}

		static const char* GetCacheDirectory()
		{
			return "assets/cache";
		}

		static void CreateCacheDirectoryIfRequired()
		{
			std::string cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
				std::filesystem::create_directories(cacheDirectory);
		}

		static const char* GLShaderStageCachedVulkanFileExtension(ShaderType stage)
		{
			switch (stage)
			{
			case ShaderType::Vertex:   return ".cache_vert.spv";
			case ShaderType::Fragment: return ".cache_frag.spv";
			case ShaderType::Geometry: return ".cache_geom.spv";
			}

			VK_CORE_ASSERT(false, "Cannot find Shader Type!");
			return "";
		}
	}

	Shader::Shader(const std::string& vsfilepath, const std::string& fsfilepath, const std::string& gsfilepath)
		: m_VertexFilePath(vsfilepath), m_FragmentFilePath(fsfilepath), m_GeometryFilePath(gsfilepath)
	{
		if (m_GeometryFilePath.empty())
		{
			auto [VertexSrc, FragmentSrc] = ParseShader(vsfilepath, fsfilepath);

			Utils::CreateCacheDirectoryIfRequired();

			std::unordered_map<uint32_t, std::string> Sources;
			Sources[(uint32_t)ShaderType::Vertex] = VertexSrc;
			Sources[(uint32_t)ShaderType::Fragment] = FragmentSrc;

			m_ShaderSources = Sources;
			CompileOrGetVulkanBinaries(Sources);
		}

		else
		{
			auto [VertexSrc, FragmentSrc, GeometrySrc] = ParseShader(vsfilepath, fsfilepath, gsfilepath);

			m_HasGeometryShader = true;
			Utils::CreateCacheDirectoryIfRequired();

			std::unordered_map<uint32_t, std::string> Sources;
			Sources[(uint32_t)ShaderType::Vertex] = VertexSrc;
			Sources[(uint32_t)ShaderType::Fragment] = FragmentSrc;
			Sources[(uint32_t)ShaderType::Geometry] = GeometrySrc;

			m_ShaderSources = Sources;
			CompileOrGetVulkanBinaries(Sources);
		}

		ReflectShaderData();
	}

	Shader::~Shader()
	{

	}

	std::shared_ptr<VulkanDescriptorSetLayout> Shader::CreateDescriptorSetLayout()
	{
		DescriptorSetLayoutBuilder descriptorSetLayoutBuilder = DescriptorSetLayoutBuilder();

		for (auto&& [stage, source] : m_VulkanSPIRV)
		{
			SpvReflectShaderModule shaderModule = {};

			SpvReflectResult result = spvReflectCreateShaderModule(
				source.size() * sizeof(uint32_t),
				source.data(),
				&shaderModule);

			VK_CORE_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "Failed to Generate Reflection Result!");

			uint32_t count = 0;
			result = spvReflectEnumerateDescriptorSets(&shaderModule, &count, nullptr);
			VK_CORE_ASSERT(count <= 1, "More than one Descriptor Sets are not supported yet!");

			std::vector<SpvReflectDescriptorSet*> DescriptorSets(count);
			result = spvReflectEnumerateDescriptorSets(&shaderModule, &count, DescriptorSets.data());

			for (uint32_t i = 0; i < count; ++i)
			{
				const SpvReflectDescriptorSet& reflectionSet = *(DescriptorSets.at(i));
				for (uint32_t j = 0; j < reflectionSet.binding_count; ++j)
				{
					const SpvReflectDescriptorBinding& reflectionBinding = *(reflectionSet.bindings[j]);

					uint32_t arrayCount = 1;
					for (uint32_t k = 0; k < reflectionBinding.array.dims_count; ++k)
						arrayCount *= reflectionBinding.array.dims[k];

					VkShaderStageFlags shaderStageFlags = 0;

					if (reflectionBinding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
						shaderStageFlags |= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

					if (reflectionBinding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
						shaderStageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;

					descriptorSetLayoutBuilder.AddBinding(
						reflectionBinding.binding,
						(VkDescriptorType)reflectionBinding.descriptor_type,
						shaderStageFlags,
						arrayCount);
				}
			}

			spvReflectDestroyShaderModule(&shaderModule);
		}

		return descriptorSetLayoutBuilder.Build();
	}

	std::tuple<std::string, std::string> Shader::ParseShader(const std::string& vsfilepath, const std::string& fsfilepath)
	{
		std::ifstream VertexSource(vsfilepath, std::ios::binary);
		std::ifstream FragmentSource(fsfilepath, std::ios::binary);

		VK_CORE_ASSERT(VertexSource.is_open(), "Failed to Open Vertex Shader File!");
		VK_CORE_ASSERT(FragmentSource.is_open(), "Failed to Open Fragment Shader File!");

		std::stringstream VertexStream, FragmentStream;

		VertexStream << VertexSource.rdbuf();
		FragmentStream << FragmentSource.rdbuf();

		return { VertexStream.str(), FragmentStream.str() };
	}

	std::tuple<std::string, std::string, std::string> Shader::ParseShader(const std::string& vsfilepath, const std::string& fsfilepath, const std::string& gsfilepath)
	{
		std::ifstream VertexSource(vsfilepath, std::ios::binary);
		std::ifstream FragmentSource(fsfilepath, std::ios::binary);
		std::ifstream GeometrySource(gsfilepath, std::ios::binary);

		VK_CORE_ASSERT(VertexSource.is_open(), "Failed to Open Vertex Shader File!");
		VK_CORE_ASSERT(FragmentSource.is_open(), "Failed to Open Fragment Shader File!");
		VK_CORE_ASSERT(GeometrySource.is_open(), "Failed to Open Geometry Shader File!");

		std::stringstream VertexStream, FragmentStream, GeometryStream;

		VertexStream << VertexSource.rdbuf();
		FragmentStream << FragmentSource.rdbuf();
		GeometryStream << GeometrySource.rdbuf();

		return { VertexStream.str(), FragmentStream.str(), GeometryStream.str() };
	}

	void Shader::CompileOrGetVulkanBinaries(const std::unordered_map<uint32_t, std::string>& shaderSources)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		const bool optimize = true;

		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		else
			options.SetOptimizationLevel(shaderc_optimization_level_zero);

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

		auto& shaderData = m_VulkanSPIRV;
		shaderData.clear();

		std::mutex MapMutexLock;

		auto CreateShader = [&](const std::filesystem::path& shaderFilePath, const std::string& source, ShaderType stage)
		{
			Timer timer(Utils::GLShaderTypeToString(stage) + " Shader Creation");

			shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, Utils::GLShaderStageToShaderC(stage), shaderFilePath.string().c_str(), options);

			VK_CORE_ASSERT(module.GetCompilationStatus() == shaderc_compilation_status_success,
				"{0} Shader: {1}", Utils::GLShaderTypeToString(stage), module.GetErrorMessage());

			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.stem().string() + Utils::GLShaderStageCachedVulkanFileExtension(stage));

			std::scoped_lock ShaderMutexLock(MapMutexLock);
			shaderData[(uint32_t)stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

			std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
			if (out.is_open())
			{
				auto& data = shaderData[(uint32_t)stage];
				out.write((char*)data.data(), data.size() * sizeof(uint32_t));
				out.flush();
				out.close();
			}
		};

		bool ShaderCreationRequired = false;

		Timer timer("Whole Shader Creation Process");

		for (auto&& [stage, source] : shaderSources)
		{
			ShaderType shaderType = (ShaderType)stage;

			std::filesystem::path shaderFilePath;

			switch (shaderType)
			{
			case ShaderType::Vertex:
				shaderFilePath = m_VertexFilePath;
				break;
			case ShaderType::Fragment:
				shaderFilePath = m_FragmentFilePath;
				break;
			case ShaderType::Geometry:
				shaderFilePath = m_GeometryFilePath;
				break;
			default:
				VK_CORE_ASSERT(false, "Cannot find Shader Type!");
				break;
			}

			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.stem().string() + Utils::GLShaderStageCachedVulkanFileExtension(shaderType));

			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);

			if (in.is_open())
			{
				in.seekg(0, std::ios::end);
				auto size = in.tellg();
				in.seekg(0, std::ios::beg);

				auto& data = shaderData[stage];
				data.resize(size / sizeof(uint32_t));
				in.read((char*)data.data(), size);
			}

			else
			{
				ShaderCreationRequired = true;
				m_Futures.push_back(std::async(std::launch::async, CreateShader, shaderFilePath, source, shaderType));
			}
		}

		for (auto& future : m_Futures)
			future.wait();

		if (ShaderCreationRequired)
			VK_CORE_TRACE("Total Shader Async Threads: {0}", m_Futures.size());
	}

	void Shader::ReflectShaderData()
	{
		std::filesystem::path shaderFilePath = m_VertexFilePath;
		VK_CORE_INFO("In {0}:", shaderFilePath.stem());

		for (auto&& [stage, shader] : m_VulkanSPIRV)
		{
			spirv_cross::Compiler compiler(shader);
			spirv_cross::ShaderResources resources = compiler.get_shader_resources();

			std::string ShaderStageType = Utils::GLShaderTypeToString((ShaderType)stage);
			VK_CORE_TRACE("  {0} Shader Reflection:", ShaderStageType);
			VK_CORE_TRACE("\t  {0} uniform buffers", resources.uniform_buffers.size());
			VK_CORE_TRACE("\t  {0} resources", resources.sampled_images.size());

			VK_CORE_TRACE("\tUniform buffers:");
			for (const auto& resource : resources.uniform_buffers)
			{
				const auto& bufferType = compiler.get_type(resource.base_type_id);
				size_t bufferSize = compiler.get_declared_struct_size(bufferType);
				uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
				size_t memberCount = bufferType.member_types.size();

				VK_CORE_TRACE("\t{0}", resource.name);
				VK_CORE_TRACE("\t  Size = {0}", bufferSize);
				VK_CORE_TRACE("\t  Binding = {0}", binding);
				VK_CORE_TRACE("\t  Members = {0}", memberCount);
			}

			VK_CORE_TRACE("\tPush Constant Data:");
			for (const auto& resource : resources.push_constant_buffers)
			{
				const auto& bufferType = compiler.get_type(resource.base_type_id);
				size_t bufferSize = compiler.get_declared_struct_size(bufferType);
				size_t memberCount = bufferType.member_types.size();

				VK_CORE_TRACE("\t  Size = {0}", bufferSize);
				VK_CORE_TRACE("\t  Members = {0}", memberCount);
			}
		}
	}

}