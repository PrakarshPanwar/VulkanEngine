#include "vulkanpch.h"
#include "VulkanRayTraceShader.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Timer.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include "../Source/SPIRV-Reflect/spirv_reflect.h"

namespace VulkanCore {

	namespace Utils {

		static std::map<std::filesystem::path, ShaderType> s_ShaderExtensionMap = {
			{ ".rgen",  ShaderType::RayGeneration },
			{ ".rahit", ShaderType::RayAnyHit },
			{ ".rchit", ShaderType::RayClosestHit },
			{ ".rmiss", ShaderType::RayMiss },
			{ ".rint",  ShaderType::RayIntersection }
		};

		static shaderc_shader_kind GLShaderStageToShaderC(ShaderType stage)
		{
			switch (stage)
			{
			case ShaderType::RayGeneration:	  return shaderc_glsl_raygen_shader;
			case ShaderType::RayAnyHit:		  return shaderc_glsl_anyhit_shader;
			case ShaderType::RayClosestHit:   return shaderc_glsl_closesthit_shader;
			case ShaderType::RayMiss:		  return shaderc_glsl_miss_shader;
			case ShaderType::RayIntersection: return shaderc_glsl_intersection_shader;
			}

			VK_CORE_ASSERT(false, "Cannot find Shader Type!");
			return (shaderc_shader_kind)0;
		}

		static std::string GLShaderTypeToString(ShaderType stage)
		{
			switch (stage)
			{
			case ShaderType::RayGeneration:	  return "RayGen";
			case ShaderType::RayAnyHit:		  return "RayAnyHit";
			case ShaderType::RayClosestHit:   return "RayClosestHit";
			case ShaderType::RayMiss:		  return "RayMiss";
			case ShaderType::RayIntersection: return "RayIntersection";
			}

			VK_CORE_ASSERT(false, "Cannot find Shader Type!");
			return {};
		}

		static const char* GetCacheDirectory()
		{
			return "assets\\cache";
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
			case ShaderType::RayGeneration:	  return ".rgen.spv";
			case ShaderType::RayAnyHit:		  return ".rahit.spv";
			case ShaderType::RayClosestHit:   return ".rchit.spv";
			case ShaderType::RayMiss:		  return ".rmiss.spv";
			case ShaderType::RayIntersection: return ".rint.spv";
			}

			VK_CORE_ASSERT(false, "Cannot find Shader Type!");
			return "";
		}

	}

	VulkanRayTraceShader::VulkanRayTraceShader(const std::string& rayGenPath, const std::string& rayClosestHitPath, const std::string& rayMissPath)
		: m_RayGenFilePath(rayGenPath), m_RayClosestHitFilePaths{ rayClosestHitPath }, m_RayMissFilePaths{ rayMissPath }
	{
		auto [RayGenSrc, RayHitSrc, RayMissSrc] = ParseShader(rayGenPath, rayClosestHitPath, rayMissPath);

		Utils::CreateCacheDirectoryIfRequired();

		std::unordered_map<std::filesystem::path, std::string> Sources;
		Sources[rayGenPath] = RayGenSrc;
		Sources[rayClosestHitPath] = RayHitSrc;
		Sources[rayMissPath] = RayMissSrc;

		m_ShaderSources = Sources;
		CompileOrGetVulkanBinaries(Sources);

		ReflectShaderData();
	}

	// TODO: Will be handled in future
	VulkanRayTraceShader::VulkanRayTraceShader(const std::string& rayGenPath, const std::string& rayClosestHitPath, const std::string& rayAnyHitPath, const std::string& rayIntersectionPath, const std::string& rayMissPath)
	{

	}

	VulkanRayTraceShader::VulkanRayTraceShader(const std::string& rayGenPath, const std::vector<std::string>& rayClosestHitPaths, const std::vector<std::string>& rayAnyHitPaths, const std::vector<std::string>& rayIntersectionPaths, const std::vector<std::string>& rayMissPaths)
		: m_RayGenFilePath(rayGenPath), m_RayClosestHitFilePaths(rayClosestHitPaths), m_RayAnyHitFilePaths(rayAnyHitPaths), m_RayIntersectionFilePaths(rayIntersectionPaths), m_RayMissFilePaths(rayMissPaths)
	{
		ParseShader();

		// If directory is not found
		Utils::CreateCacheDirectoryIfRequired();

		CompileOrGetVulkanBinaries(m_ShaderSources);
		ReflectShaderData();
	}

	VulkanRayTraceShader::~VulkanRayTraceShader()
	{

	}

	std::shared_ptr<VulkanDescriptorSetLayout> VulkanRayTraceShader::CreateDescriptorSetLayout(int index)
	{
		DescriptorSetLayoutBuilder descriptorSetLayoutBuilder = DescriptorSetLayoutBuilder();

		for (auto&& [shaderPath, source] : m_VulkanSPIRV)
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

						if (reflectionBinding.descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
							shaderStageFlags |= VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;

						if (reflectionBinding.descriptor_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
							shaderStageFlags |= VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;

						if (reflectionBinding.descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
							shaderStageFlags |= VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR;

						if (reflectionBinding.descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
							shaderStageFlags |= VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

						if (reflectionBinding.descriptor_type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
							shaderStageFlags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

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
		m_DescriptorSetsLayout.push_back(descriptorSetLayout);

		return descriptorSetLayout;
	}

	std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> VulkanRayTraceShader::CreateAllDescriptorSetsLayout()
	{
		// Key: Set number
		std::map<uint32_t, DescriptorSetLayoutBuilder> descriptorSetLayoutBuilderMap;

		for (auto&& [shaderPath, source] : m_VulkanSPIRV)
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

					if (reflectionBinding.descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
						shaderStageFlags |= VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;

					if (reflectionBinding.descriptor_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
						shaderStageFlags |= VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;

					if (reflectionBinding.descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
						shaderStageFlags |= VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR;

					if (reflectionBinding.descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
						shaderStageFlags |= VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

					if (reflectionBinding.descriptor_type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
						shaderStageFlags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

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
			m_DescriptorSetsLayout.push_back(descriptorSetLayoutBuilder.Build());

		return m_DescriptorSetsLayout;
	}

	void VulkanRayTraceShader::Reload()
	{
		ParseShader();

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		options.SetTargetSpirv(shaderc_spirv_version_1_4);
		const bool optimize = true;

		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);
		else
			options.SetOptimizationLevel(shaderc_optimization_level_zero);

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

		auto& shaderSources = m_ShaderSources;
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
			shaderData[shaderFilePath] = std::vector<uint32_t>(module.cbegin(), module.cend());

			std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
			if (out.is_open())
			{
				auto& data = shaderData[shaderFilePath];
				out.write((char*)data.data(), data.size() * sizeof(uint32_t));
				out.flush();
				out.close();
			}
		};

		Timer timer("Whole Shader Creation Process");

		for (auto&& [shaderPath, source] : shaderSources)
		{
			ShaderType shaderType = Utils::s_ShaderExtensionMap[shaderPath.extension()];

			std::filesystem::path cachedPath = cacheDirectory / (shaderPath.stem().string() + Utils::GLShaderStageCachedVulkanFileExtension(shaderType));
			m_Futures.push_back(std::async(std::launch::async, CreateShader, shaderPath, source, shaderType));
		}

		SetReloadFlag();
		VK_CORE_TRACE("Total Shader Async Threads: {0}", m_Futures.size());

		for (auto& future : m_Futures)
			future.wait();
	}

	std::tuple<std::string, std::string, std::string> VulkanRayTraceShader::ParseShader(const std::string& rayGenPath, const std::string& rayClosestHitPath, const std::string& rayMissPath)
	{
		std::ifstream RayGenSource(rayGenPath, std::ios::binary);
		std::ifstream RayClosestHitSource(rayClosestHitPath, std::ios::binary);
		std::ifstream RayMissSource(rayMissPath, std::ios::binary);

		VK_CORE_ASSERT(RayGenSource.is_open(), "Failed to Open Ray Generation Shader File!");
		VK_CORE_ASSERT(RayClosestHitSource.is_open(), "Failed to Open Ray Closest Hit Shader File!");
		VK_CORE_ASSERT(RayMissSource.is_open(), "Failed to Open Ray Miss Shader File!");

		std::stringstream RayGenStream, RayClosestHitStream, RayMissStream;

		RayGenStream << RayGenSource.rdbuf();
		RayClosestHitStream << RayClosestHitSource.rdbuf();
		RayMissStream << RayMissSource.rdbuf();

		return { RayGenStream.str(), RayClosestHitStream.str(), RayMissStream.str() };
	}

	void VulkanRayTraceShader::ParseShader()
	{
		std::unordered_map<std::filesystem::path, std::string> Sources;

		// Ray Generation
		{
			std::ifstream RayGenSource(m_RayGenFilePath, std::ios::binary);

			VK_CORE_ASSERT(RayGenSource.is_open(), "Failed to Open Ray Generation Shader File!");

			std::stringstream RayGenStream;
			RayGenStream << RayGenSource.rdbuf();

			Sources[m_RayGenFilePath] = RayGenStream.str();
		}

		// Closest Hit
		{
			for (auto& rayClosestHitFilePath : m_RayClosestHitFilePaths)
			{
				std::ifstream RayClosestHitSource(rayClosestHitFilePath, std::ios::binary);

				VK_CORE_ASSERT(RayClosestHitSource.is_open(), "Failed to Open Ray Closest Hit Shader File!");

				std::stringstream RayClosestHitStream;
				RayClosestHitStream << RayClosestHitSource.rdbuf();

				Sources[rayClosestHitFilePath] = RayClosestHitStream.str();
			}
		}

		// Any Hit
		{
			for (auto& rayAnyHitFilePath : m_RayAnyHitFilePaths)
			{
				std::ifstream RayAnyHitSource(rayAnyHitFilePath, std::ios::binary);

				VK_CORE_ASSERT(RayAnyHitSource.is_open(), "Failed to Open Ray Any Hit Shader File!");

				std::stringstream RayAnyHitStream;
				RayAnyHitStream << RayAnyHitSource.rdbuf();

				Sources[rayAnyHitFilePath] = RayAnyHitStream.str();
			}
		}

		// Intersection
		{
			for (auto& rayIntersectionFilePath : m_RayIntersectionFilePaths)
			{
				std::ifstream RayIntersectionSource(rayIntersectionFilePath, std::ios::binary);

				VK_CORE_ASSERT(RayIntersectionSource.is_open(), "Failed to Open Ray Intersection Shader File!");

				std::stringstream RayIntersectionStream;
				RayIntersectionStream << RayIntersectionSource.rdbuf();

				Sources[rayIntersectionFilePath] = RayIntersectionStream.str();
			}
		}

		// Miss
		{
			for (auto& rayMissFilePath : m_RayMissFilePaths)
			{
				std::ifstream RayMissSource(rayMissFilePath, std::ios::binary);

				VK_CORE_ASSERT(RayMissSource.is_open(), "Failed to Open Ray Miss Shader File!");

				std::stringstream RayMissStream;
				RayMissStream << RayMissSource.rdbuf();

				Sources[rayMissFilePath] = RayMissStream.str();
			}
		}

		m_ShaderSources = Sources;
	}

	void VulkanRayTraceShader::CompileOrGetVulkanBinaries(const std::unordered_map<std::filesystem::path, std::string>& shaderSources)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		options.SetTargetSpirv(shaderc_spirv_version_1_4);
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

			if (module.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				VK_CORE_CRITICAL("{0} Shader: {1}", Utils::GLShaderTypeToString(stage), module.GetErrorMessage());
				__debugbreak();
			}

			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.stem().string() + Utils::GLShaderStageCachedVulkanFileExtension(stage));

			std::scoped_lock ShaderMutexLock(MapMutexLock);
			shaderData[shaderFilePath] = std::vector<uint32_t>(module.cbegin(), module.cend());

			std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
			if (out.is_open())
			{
				auto& data = shaderData[shaderFilePath];
				out.write((char*)data.data(), data.size() * sizeof(uint32_t));
				out.flush();
				out.close();
			}
		};

		Timer timer("Whole Shader Creation Process");

		for (auto&& [shaderPath, source] : shaderSources)
		{
			ShaderType shaderType = Utils::s_ShaderExtensionMap[shaderPath.extension()];

			std::filesystem::path cachedPath = cacheDirectory / (shaderPath.stem().string() + Utils::GLShaderStageCachedVulkanFileExtension(shaderType));

			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open())
			{
				in.seekg(0, std::ios::end);
				auto size = in.tellg();
				in.seekg(0, std::ios::beg);

				auto& data = shaderData[shaderPath];
				data.resize(size / sizeof(uint32_t));
				in.read((char*)data.data(), size);
			}
			else
				m_Futures.push_back(std::async(std::launch::async, CreateShader, shaderPath, source, shaderType));
		}

		if (!m_Futures.empty())
			VK_CORE_TRACE("Total Shader Async Threads: {0}", m_Futures.size());

		for (auto& future : m_Futures)
			future.wait();
	}

	void VulkanRayTraceShader::ReflectShaderData()
	{
		std::filesystem::path shaderFilePath = m_RayGenFilePath;

		VK_CORE_INFO("In {0}:", shaderFilePath.stem());
		for (auto&& [shaderPath, shader] : m_VulkanSPIRV)
		{
			spirv_cross::Compiler compiler(shader);
			spirv_cross::ShaderResources resources = compiler.get_shader_resources();

			std::string ShaderStageType = Utils::GLShaderTypeToString(Utils::s_ShaderExtensionMap[shaderPath.extension()]);
			VK_CORE_TRACE("  {0} Shader Reflection:", ShaderStageType);
			VK_CORE_TRACE("\t  {0} uniform buffers", resources.uniform_buffers.size());
			VK_CORE_TRACE("\t  {0} storage buffers", resources.storage_buffers.size());
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

			VK_CORE_TRACE("\tStorage buffers:");
			for (const auto& resource : resources.storage_buffers)
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

}
