#include "vulkanpch.h"
#include "VulkanShader.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Core/Timer.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "Platform/Vulkan/VulkanSwapChain.h"
#include "Platform/Vulkan/VulkanDescriptor.h"

#include <regex>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include "../Source/SPIRV-Reflect/spirv_reflect.h"

namespace VulkanCore {

	namespace Utils {

		static std::map<std::filesystem::path, ShaderType> s_ShaderExtensionMap = {
			{ ".vert", ShaderType::Vertex },
			{ ".frag", ShaderType::Fragment },
			{ ".tesc", ShaderType::TessellationControl },
			{ ".tese", ShaderType::TessellationEvaluation },
			{ ".geom", ShaderType::Geometry },
			{ ".comp", ShaderType::Compute }
		};

		static shaderc_shader_kind GLShaderStageToShaderC(ShaderType stage)
		{
			switch (stage)
			{
			case ShaderType::Vertex:				 return shaderc_glsl_vertex_shader;
			case ShaderType::Fragment:				 return shaderc_glsl_fragment_shader;
			case ShaderType::TessellationControl:	 return shaderc_glsl_tess_control_shader;
			case ShaderType::TessellationEvaluation: return shaderc_glsl_tess_evaluation_shader;
			case ShaderType::Geometry:				 return shaderc_glsl_geometry_shader;
			case ShaderType::Compute:				 return shaderc_glsl_compute_shader;
			}

			VK_CORE_ASSERT(false, "Cannot find Shader Type!");
			return (shaderc_shader_kind)0;
		}

		static std::string GLShaderTypeToString(ShaderType stage)
		{
			switch (stage)
			{
			case ShaderType::Vertex:				 return "Vertex";
			case ShaderType::Fragment:				 return "Fragment";
			case ShaderType::TessellationControl:	 return "TessellationControl";
			case ShaderType::TessellationEvaluation: return "TessellationEvaluation";
			case ShaderType::Geometry:				 return "Geometry";
			case ShaderType::Compute:				 return "Compute";
			}

			VK_CORE_ASSERT(false, "Cannot find Shader Type!");
			return {};
		}

		static const char* GetCacheDirectory()
		{
			return "cache";
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
			case ShaderType::Vertex:				 return ".vert.spv";
			case ShaderType::Fragment:				 return ".frag.spv";
			case ShaderType::TessellationControl:	 return ".tesc.spv";
			case ShaderType::TessellationEvaluation: return ".tese.spv";
			case ShaderType::Geometry:				 return ".geom.spv";
			case ShaderType::Compute:				 return ".comp.spv";
			}

			VK_CORE_ASSERT(false, "Cannot find Shader Type!");
			return "";
		}

	}

	VulkanShader::VulkanShader(const std::string& vertexPath, const std::string& fragmentPath, const std::string& geometryPath, const std::string& tessellationControlPath, const std::string& tessellationEvaluationPath)
		: m_VertexFilePath(vertexPath), m_FragmentFilePath(fragmentPath), m_GeometryFilePath(geometryPath),
		m_TessellationControlFilePath(tessellationControlPath), m_TessellationEvaluationFilePath(tessellationEvaluationPath)
	{
		auto [VertexSrc, FragmentSrc] = ParseShader(vertexPath, fragmentPath);

		Utils::CreateCacheDirectoryIfRequired();

		std::unordered_map<uint32_t, std::string> Sources;
		Sources[(uint32_t)ShaderType::Vertex] = VertexSrc;
		Sources[(uint32_t)ShaderType::Fragment] = FragmentSrc;

		if (!m_GeometryFilePath.empty())
		{
			auto GeometrySrc = ParseShader(geometryPath);
			Sources[(uint32_t)ShaderType::Geometry] = GeometrySrc;
		}

		if (!m_TessellationControlFilePath.empty())
		{
			auto TessellationControlSrc = ParseShader(tessellationControlPath);
			Sources[(uint32_t)ShaderType::TessellationControl] = TessellationControlSrc;
		}

		if (!m_TessellationEvaluationFilePath.empty())
		{
			auto TessellationEvaluationSrc = ParseShader(m_TessellationEvaluationFilePath);
			Sources[(uint32_t)ShaderType::TessellationEvaluation] = TessellationEvaluationSrc;
		}

		CompileOrGetVulkanBinaries(Sources);
		ReflectShaderData();
		InvalidateDescriptors();
	}

	VulkanShader::VulkanShader(const std::string& computePath)
		: m_ComputeFilePath(computePath)
	{
		auto ComputeSrc = ParseShader(computePath);

		Utils::CreateCacheDirectoryIfRequired();

		std::unordered_map<uint32_t, std::string> Sources;
		Sources[(uint32_t)ShaderType::Compute] = ComputeSrc;

		CompileOrGetVulkanBinaries(Sources);
		ReflectShaderData();
		InvalidateDescriptors();
	}

	VulkanShader::~VulkanShader()
	{
	}

	std::shared_ptr<VulkanDescriptorSetLayout> VulkanShader::CreateDescriptorSetLayout(int index)
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
				if (index == reflectionSet.set)
				{
					for (uint32_t j = 0; j < reflectionSet.binding_count; ++j)
					{
						const SpvReflectDescriptorBinding& reflectionBinding = *(reflectionSet.bindings[j]);

						uint32_t arrayCount = 1;
						for (uint32_t k = 0; k < reflectionBinding.array.dims_count; ++k)
							arrayCount *= reflectionBinding.array.dims[k];

						VkShaderStageFlags shaderStageFlags = 0;

						if (reflectionBinding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
							shaderStageFlags |= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

						if (reflectionBinding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
							shaderStageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

						if (reflectionBinding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE)
							shaderStageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;

						if (reflectionBinding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
							shaderStageFlags |= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

						descriptorSetLayoutBuilder.AddBinding(
							reflectionBinding.binding,
							(VkDescriptorType)reflectionBinding.descriptor_type,
							shaderStageFlags,
							arrayCount);
					}

					break;
				}
			}

			spvReflectDestroyShaderModule(&shaderModule);
		}

		auto descriptorSetLayout = descriptorSetLayoutBuilder.Build();
		m_DescriptorSetLayouts.push_back(descriptorSetLayout);

		return descriptorSetLayout;
	}

	std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> VulkanShader::CreateAllDescriptorSetsLayout()
	{
		// Key: Set number
		std::unordered_map<uint32_t, DescriptorSetLayoutBuilder> descriptorSetLayoutBuilderMap;

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
						shaderStageFlags |= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

					if (reflectionBinding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
						shaderStageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

					if (reflectionBinding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE)
						shaderStageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;

					if (reflectionBinding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
						shaderStageFlags |= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

					descriptorSetLayoutBuilderMap[reflectionSet.set].AddBinding(
						reflectionBinding.binding,
						(VkDescriptorType)reflectionBinding.descriptor_type,
						shaderStageFlags,
						arrayCount);
				}
			}

			spvReflectDestroyShaderModule(&shaderModule);
		}

		for (auto&& [setID, descriptorSetLayoutBuilder] : descriptorSetLayoutBuilderMap)
			m_DescriptorSetLayouts.push_back(descriptorSetLayoutBuilder.Build());

		return m_DescriptorSetLayouts;
	}

	std::vector<VkDescriptorSet> VulkanShader::AllocateDescriptorSets(uint32_t index)
	{
		auto vulkanDescriptorPool = VulkanRenderer::Get()->GetDescriptorPool();
		VkDescriptorSetLayout setLayout = CreateDescriptorSetLayout(index)->GetVulkanDescriptorSetLayout();

		std::vector<VkDescriptorSet> descriptorSets(3);
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
			vulkanDescriptorPool->AllocateDescriptorSet(setLayout, descriptorSets[i]);

		return descriptorSets;
	}

	std::vector<VkDescriptorSet> VulkanShader::AllocateAllDescriptorSets()
	{
		return {};
	}

	std::tuple<std::string, std::string> VulkanShader::ParseShader(const std::string& vertexPath, const std::string& fragmentPath)
	{
		std::ifstream VertexSource(vertexPath, std::ios::binary);
		std::ifstream FragmentSource(fragmentPath, std::ios::binary);

		VK_CORE_ASSERT(VertexSource.is_open(), "Failed to Open Vertex Shader File!");
		VK_CORE_ASSERT(FragmentSource.is_open(), "Failed to Open Fragment Shader File!");

		std::stringstream VertexStream, FragmentStream;

		VertexStream << VertexSource.rdbuf();
		FragmentStream << FragmentSource.rdbuf();

		return { ParsePreprocessIncludes(VertexStream), ParsePreprocessIncludes(FragmentStream) };
	}

#if 0
	std::tuple<std::string, std::string, std::string> VulkanShader::ParseShader(const std::string& vertexPath, const std::string& fragmentPath, const std::string& geometryPath)
	{
		std::ifstream VertexSource(vertexPath, std::ios::binary);
		std::ifstream FragmentSource(fragmentPath, std::ios::binary);
		std::ifstream GeometrySource(geometryPath, std::ios::binary);

		VK_CORE_ASSERT(VertexSource.is_open(), "Failed to Open Vertex Shader File!");
		VK_CORE_ASSERT(FragmentSource.is_open(), "Failed to Open Fragment Shader File!");
		VK_CORE_ASSERT(GeometrySource.is_open(), "Failed to Open Geometry Shader File!");

		std::stringstream VertexStream, FragmentStream, GeometryStream;

		VertexStream << VertexSource.rdbuf();
		FragmentStream << FragmentSource.rdbuf();
		GeometryStream << GeometrySource.rdbuf();

		return { VertexStream.str(), FragmentStream.str(), GeometryStream.str() };
	}
#endif

	std::string VulkanShader::ParseShader(const std::string& shaderPath)
	{
		std::ifstream ShaderSource(shaderPath, std::ios::binary);

		std::filesystem::path filePath = shaderPath;
		VK_CORE_ASSERT(ShaderSource.is_open(), "Failed to Open {0} Shader File!", Utils::GLShaderTypeToString(Utils::s_ShaderExtensionMap[filePath.extension()]));

		std::stringstream ShaderStream;
		ShaderStream << ShaderSource.rdbuf();

		return ParsePreprocessIncludes(ShaderStream);
	}

	std::unordered_map<uint32_t, std::string> VulkanShader::ParseShaders()
	{
		std::unordered_map<uint32_t, std::string> Sources;

		if (!(m_VertexFilePath.empty() || m_FragmentFilePath.empty()))
		{
			std::ifstream VertexSource(m_VertexFilePath, std::ios::binary);
			std::ifstream FragmentSource(m_FragmentFilePath, std::ios::binary);
			VK_CORE_ASSERT(VertexSource.is_open(), "Failed to Open Vertex Shader File!");
			VK_CORE_ASSERT(FragmentSource.is_open(), "Failed to Open Fragment Shader File!");

			std::stringstream VertexStream, FragmentStream;

			VertexStream << VertexSource.rdbuf();
			FragmentStream << FragmentSource.rdbuf();

			Sources[(uint32_t)ShaderType::Vertex] = ParsePreprocessIncludes(VertexStream);
			Sources[(uint32_t)ShaderType::Fragment] = ParsePreprocessIncludes(FragmentStream);

			if (std::filesystem::exists(m_GeometryFilePath))
			{
				std::ifstream GeometrySource(m_GeometryFilePath, std::ios::binary);
				
				std::stringstream GeometryStream;
				GeometryStream << GeometrySource.rdbuf();

				Sources[(uint32_t)ShaderType::Geometry] = ParsePreprocessIncludes(GeometryStream);
			}

			if (std::filesystem::exists(m_TessellationControlFilePath) && std::filesystem::exists(m_TessellationEvaluationFilePath))
			{
				std::ifstream TessellationControlSource(m_TessellationControlFilePath, std::ios::binary);
				std::ifstream TessellationEvaluationSource(m_TessellationEvaluationFilePath, std::ios::binary);

				std::stringstream TessellationControlStream, TessellationEvaluationStream;

				TessellationControlStream << TessellationControlSource.rdbuf();
				TessellationEvaluationStream << TessellationEvaluationSource.rdbuf();

				Sources[(uint32_t)ShaderType::TessellationControl] = ParsePreprocessIncludes(TessellationControlStream);
				Sources[(uint32_t)ShaderType::TessellationEvaluation] = ParsePreprocessIncludes(TessellationEvaluationStream);
			}
		}
		else
		{
			std::ifstream ComputeSource(m_ComputeFilePath, std::ios::binary);

			VK_CORE_ASSERT(ComputeSource.is_open(), "Failed to Open Compute Shader File!");

			std::stringstream ComputeStream;

			ComputeStream << ComputeSource.rdbuf();

			Sources[(uint32_t)ShaderType::Compute] = ParsePreprocessIncludes(ComputeStream);
		}

		return Sources;
	}

	// NOTE: It currently supports single depth headers(i.e. no header within header)
	std::string VulkanShader::ParsePreprocessIncludes(std::stringstream& sourceCode)
	{
		const std::filesystem::path shaderPath = "shaders";

		std::string sourceStr = sourceCode.str();
		std::regex includeRegex("^[ ]*#[ ]*include[ ]+[\"<](.*)[\">].*");

		std::smatch matches{};
		std::string line{};
		while (std::getline(sourceCode, line))
		{
			if (std::regex_search(line, matches, includeRegex))
			{
				std::filesystem::path includeFilePath = matches[1].str();
				includeFilePath = shaderPath / includeFilePath;

				if (std::filesystem::exists(includeFilePath))
				{
					std::ifstream includeFileSource(includeFilePath, std::ios::binary);

					std::stringstream includeFileStream;
					includeFileStream << includeFileSource.rdbuf();

					sourceStr = std::regex_replace(sourceStr, includeRegex, includeFileStream.str(), std::regex_constants::format_first_only);
				}
				else
				{
					VK_CORE_CRITICAL("{} header doesn't exist!", includeFilePath.generic_string());
					__debugbreak();
				}
			}
		}

		return sourceStr;
	}

	void VulkanShader::CompileOrGetVulkanBinaries(std::unordered_map<uint32_t, std::string>& shaderSources)
	{
		auto device = VulkanContext::GetCurrentDevice();

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		options.SetTargetSpirv(shaderc_spirv_version_1_4);

		const bool optimize = !device->IsInDebugMode();
		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);
		else
		{
			options.SetGenerateDebugInfo();
			options.SetOptimizationLevel(shaderc_optimization_level_zero);
		}

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

		auto& shaderData = m_VulkanSPIRV;
		shaderData.clear();

		std::mutex MapMutexLock;

		auto CreateShader = [&](const std::filesystem::path& shaderFilePath, const std::string& source, ShaderType stage)
		{
			Timer timer(Utils::GLShaderTypeToString(stage) + " Shader Creation");

			shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, Utils::GLShaderStageToShaderC(stage), shaderFilePath.string().c_str(), options);

			if (module.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				VK_CORE_CRITICAL("{0} Shader: {1}", Utils::GLShaderTypeToString(stage), module.GetErrorMessage());
				__debugbreak();
			}

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

		Timer timer("Whole Shader Creation Process");

		for (auto&& [stage, source] : shaderSources)
		{
			std::filesystem::path shaderFilePath;

			ShaderType shaderType = (ShaderType)stage;
			switch (shaderType)
			{
			case ShaderType::Vertex:
				shaderFilePath = m_VertexFilePath;
				break;
			case ShaderType::Fragment:
				shaderFilePath = m_FragmentFilePath;
				break;
			case ShaderType::TessellationControl:
				shaderFilePath = m_TessellationControlFilePath;
				break;
			case ShaderType::TessellationEvaluation:
				shaderFilePath = m_TessellationEvaluationFilePath;
				break;
			case ShaderType::Geometry:
				shaderFilePath = m_GeometryFilePath;
				break;
			case ShaderType::Compute:
				shaderFilePath = m_ComputeFilePath;
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
				m_Futures.push_back(std::async(std::launch::async, CreateShader, shaderFilePath, source, shaderType));
		}

		for (auto& future : m_Futures)
			future.wait();

		if (!m_Futures.empty())
			VK_CORE_TRACE("Total Shader Async Threads: {0}", m_Futures.size());
	}

	void VulkanShader::Reload()
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto shaderSources = ParseShaders();

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		options.SetTargetSpirv(shaderc_spirv_version_1_4);

		const bool optimize = !device->IsInDebugMode();
		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);
		else
		{
			options.SetGenerateDebugInfo();
			options.SetOptimizationLevel(shaderc_optimization_level_zero);
		}

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

		auto& shaderData = m_VulkanSPIRV;
		shaderData.clear();
		m_Futures.clear();

		std::mutex MapMutexLock;

		auto CreateShader = [&](const std::filesystem::path& shaderFilePath, const std::string& source, ShaderType stage)
		{
			Timer timer(Utils::GLShaderTypeToString(stage) + " Shader Creation");

			shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, Utils::GLShaderStageToShaderC(stage), shaderFilePath.string().c_str(), options);

			if (module.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				VK_CORE_CRITICAL("{0} Shader: {1}", Utils::GLShaderTypeToString(stage), module.GetErrorMessage());
				__debugbreak();
			}

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

		Timer timer("Shader Recreation Process");

		for (auto&& [stage, source] : shaderSources)
		{
			std::filesystem::path shaderFilePath;

			ShaderType shaderType = (ShaderType)stage;
			switch (shaderType)
			{
			case ShaderType::Vertex:
				shaderFilePath = m_VertexFilePath;
				break;
			case ShaderType::Fragment:
				shaderFilePath = m_FragmentFilePath;
				break;
			case ShaderType::TessellationControl:
				shaderFilePath = m_TessellationControlFilePath;
				break;
			case ShaderType::TessellationEvaluation:
				shaderFilePath = m_TessellationEvaluationFilePath;
				break;
			case ShaderType::Geometry:
				shaderFilePath = m_GeometryFilePath;
				break;
			case ShaderType::Compute:
				shaderFilePath = m_ComputeFilePath;
				break;
			default:
				VK_CORE_ASSERT(false, "Cannot find Shader Type!");
				break;
			}

			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.stem().string() + Utils::GLShaderStageCachedVulkanFileExtension(shaderType));
			m_Futures.push_back(std::async(std::launch::async, CreateShader, shaderFilePath, source, shaderType));
		}

		for (auto& future : m_Futures)
			future.wait();

		VK_CORE_TRACE("Total Shader Async Threads: {0}", m_Futures.size());
		SetReloadFlag();
	}

	void VulkanShader::ReflectShaderData()
	{
		std::filesystem::path shaderFilePath = m_VertexFilePath;
		if (!std::filesystem::exists(shaderFilePath))
			shaderFilePath = m_ComputeFilePath;

		VK_CORE_INFO("In {0}:", shaderFilePath.stem().string());

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
				m_PushConstantSize = bufferSize;

				VK_CORE_TRACE("\t  Size = {0}", bufferSize);
				VK_CORE_TRACE("\t  Members = {0}", memberCount);
			}
		}
	}

	void VulkanShader::InvalidateDescriptors()
	{

	}

}
