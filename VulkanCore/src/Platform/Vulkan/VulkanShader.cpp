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

		static constexpr std::array<ShaderType, 6> s_ShaderTypes = {
			ShaderType::Compute,
			ShaderType::Vertex,
			ShaderType::Fragment,
			ShaderType::TessellationControl,
			ShaderType::TessellationEvaluation,
			ShaderType::Geometry
		};

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

		static consteval const char* GetCacheDirectory()
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

	VulkanShader::VulkanShader(const std::string& shaderName)
		: m_ShaderName(shaderName)
	{
		Utils::CreateCacheDirectoryIfRequired();

		Timer timer("GLSL Shader Creation");

		CheckRequiredStagesForCompilation();

		std::unordered_map<uint32_t, std::tuple<std::filesystem::path, std::string>> Sources = ParseShaders();
		CompileOrGetVulkanBinaries(Sources);

		ReflectShaderData();
		//InvalidateDescriptors();
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

	void VulkanShader::CheckRequiredStagesForCompilation()
	{
		constexpr const char* cacheDirectory = "cache/", *shaderDirectory = "shaders/";

		const std::array<std::pair<std::string, std::string>, 6> shaderPaths = {
			std::make_pair(shaderDirectory + m_ShaderName + ".comp", cacheDirectory + m_ShaderName + ".comp.spv"),
			std::make_pair(shaderDirectory + m_ShaderName + ".vert", cacheDirectory + m_ShaderName + ".vert.spv"),
			std::make_pair(shaderDirectory + m_ShaderName + ".frag", cacheDirectory + m_ShaderName + ".frag.spv"),
			std::make_pair(shaderDirectory + m_ShaderName + ".tesc", cacheDirectory + m_ShaderName + ".tesc.spv"),
			std::make_pair(shaderDirectory + m_ShaderName + ".tese", cacheDirectory + m_ShaderName + ".tese.spv"),
			std::make_pair(shaderDirectory + m_ShaderName + ".geom", cacheDirectory + m_ShaderName + ".geom.spv")
		};

		for (uint32_t i = 0; auto&& [shaderPath, cachePath] : shaderPaths)
		{
			if (std::filesystem::exists(shaderPath) && !std::filesystem::exists(cachePath))
				m_LoadShaderFlag.set(i);
			else if (std::filesystem::exists(cachePath))
				m_LoadShaderFlag.set(i + 1);

			i += 2;
		}
	}

	std::unordered_map<uint32_t, std::tuple<std::filesystem::path, std::string>> VulkanShader::ParseShaders()
	{
		constexpr const char* shaderDirectory = "shaders/";

		std::unordered_map<uint32_t, std::tuple<std::filesystem::path, std::string>> Sources;

		const std::array<std::pair<ShaderType, std::string>, 6> shaderPaths = {
			std::make_pair(ShaderType::Compute,				   shaderDirectory + m_ShaderName + ".comp"),
			std::make_pair(ShaderType::Vertex,				   shaderDirectory + m_ShaderName + ".vert"),
			std::make_pair(ShaderType::Fragment,			   shaderDirectory + m_ShaderName + ".frag"),
			std::make_pair(ShaderType::TessellationControl,	   shaderDirectory + m_ShaderName + ".tesc"),
			std::make_pair(ShaderType::TessellationEvaluation, shaderDirectory + m_ShaderName + ".tese"),
			std::make_pair(ShaderType::Geometry,			   shaderDirectory + m_ShaderName + ".geom")
		};

		for (uint32_t i = 0; auto&& [stage, shaderPath] : shaderPaths)
		{
			if (m_LoadShaderFlag.test(i))
			{
				std::ifstream ShaderSource(shaderPath, std::ios::binary);

				std::filesystem::path filePath = shaderPath;
				VK_CORE_ASSERT(ShaderSource.is_open(), "Failed to Open {0} Shader File!", Utils::GLShaderTypeToString(Utils::s_ShaderExtensionMap[filePath.extension()]));

				std::stringstream ShaderStream;
				ShaderStream << ShaderSource.rdbuf();

				Sources[(uint32_t)stage] = { filePath, ParsePreprocessIncludes(ShaderStream) };
			}

			i += 2;
		}

		return Sources;
	}

	// NOTE: It currently supports single depth headers(i.e. no header within header)
	std::string VulkanShader::ParsePreprocessIncludes(std::stringstream& sourceCode)
	{
		constexpr const char* shaderPath = "shaders";

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

	void VulkanShader::CompileOrGetVulkanBinaries(std::unordered_map<uint32_t, std::tuple<std::filesystem::path, std::string>>& shaderSources)
	{
		auto device = VulkanContext::GetCurrentDevice();

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4);
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

		for (uint32_t i = 0; auto stage : Utils::s_ShaderTypes)
		{
			uint8_t lowB = m_LoadShaderFlag.test(i), highB = m_LoadShaderFlag.test(i + 1);
			uint8_t loadBit = (highB << 1) | lowB;

			switch (loadBit)
			{
			case 0:
				break;
			case 1: // Load from Shader Source File
			{
				auto& source = shaderSources[(uint32_t)stage];

				std::filesystem::path shaderFilePath = std::get<0>(source);
				ShaderType shaderType = (ShaderType)stage;

				m_Futures.push_back(std::async(std::launch::async, CreateShader, shaderFilePath, std::get<1>(source), shaderType));

				break;
			}
			case 2: // Load from Shader Cache Binary
			{
				std::filesystem::path cachedPath = cacheDirectory / (m_ShaderName + Utils::GLShaderStageCachedVulkanFileExtension(stage));

				std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
				if (in.is_open())
				{
					in.seekg(0, std::ios::end);
					auto size = in.tellg();
					in.seekg(0, std::ios::beg);

					auto& data = shaderData[(uint32_t)stage];
					data.resize(size / sizeof(uint32_t));
					in.read((char*)data.data(), size);
				}

				break;
			}
			default:
				VK_CORE_ASSERT(false, "Invalid Load Bit!");
				break;
			}

			i += 2;
		}

		for (auto& future : m_Futures)
			future.wait();

		if (!m_Futures.empty())
			VK_CORE_TRACE("Total Shader Async Threads: {0}", m_Futures.size());
	}

	void VulkanShader::Reload()
	{
		auto device = VulkanContext::GetCurrentDevice();

		Timer timer("Shader Recreation Process");

		// Check Valid Shader Types and reset Load flag
		for (uint32_t i = 0; auto stage : Utils::s_ShaderTypes)
		{
			if (m_LoadShaderFlag.test(i) || m_LoadShaderFlag.test(i + 1))
			{
				m_LoadShaderFlag.set(i);
				m_LoadShaderFlag.reset(i + 1);
			}

			i += 2;
		}

		auto shaderSources = ParseShaders();

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4);
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

		for (auto&& [stage, source] : shaderSources)
		{
			ShaderType shaderType = (ShaderType)stage;
			std::filesystem::path shaderFilePath = std::get<0>(source);

			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.stem().string() + Utils::GLShaderStageCachedVulkanFileExtension(shaderType));
			m_Futures.push_back(std::async(std::launch::async, CreateShader, shaderFilePath, std::get<1>(source), shaderType));
		}

		for (auto& future : m_Futures)
			future.wait();

		VK_CORE_TRACE("Total Shader Async Threads: {0}", m_Futures.size());
		SetReloadFlag();
	}

	bool VulkanShader::HasGeometryShader() const
	{
		return m_VulkanSPIRV.contains((uint32_t)ShaderType::Geometry);
	}

	bool VulkanShader::HasTessellationShaders() const
	{
		return m_VulkanSPIRV.contains((uint32_t)ShaderType::TessellationControl)
			&& m_VulkanSPIRV.contains((uint32_t)ShaderType::TessellationEvaluation);
	}

	void VulkanShader::ReflectShaderData()
	{
		VK_CORE_INFO("In {0}:", m_ShaderName);

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
