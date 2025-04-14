#include "vulkanpch.h"
#include "VulkanSlangShader.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Timer.h"

#include <slang/slang-com-helper.h>
#include <spirv_cross/spirv_cross.hpp>
#include "../Source/SPIRV-Reflect/spirv_reflect.h"

namespace VulkanCore {

	namespace Utils {

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

		static ShaderType SlangShaderTypeToGLShaderType(SlangStage slangType)
		{
			switch (slangType)
			{
			case SLANG_STAGE_VERTEX:   return ShaderType::Vertex;
			case SLANG_STAGE_FRAGMENT: return ShaderType::Fragment;
			case SLANG_STAGE_HULL:	   return ShaderType::TessellationControl;
			case SLANG_STAGE_DOMAIN:   return ShaderType::TessellationEvaluation;
			case SLANG_STAGE_GEOMETRY: return ShaderType::Geometry;
			case SLANG_STAGE_COMPUTE:  return ShaderType::Compute;
			}

			VK_CORE_ASSERT(false, "Cannot find Shader Type!")
			return (ShaderType)0;
		}

		static const char* GetSlangEntryPointFromType(ShaderType stage)
		{
			switch (stage)
			{
			case ShaderType::Vertex:				 return "VSMain";
			case ShaderType::Fragment:				 return "FSMain";
			case ShaderType::TessellationControl:	 return "TCSMain";
			case ShaderType::TessellationEvaluation: return "TESMain";
			case ShaderType::Geometry:				 return "GSMain";
			case ShaderType::Compute:				 return "CSMain";
			}

			VK_CORE_ASSERT(false, "Cannot find Shader Type!");
			return "";
		}

		static const char* GetCacheDirectory()
		{
			return "cache\\slang";
		}

		static void CreateCacheDirectoryIfRequired()
		{
			std::string cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
				std::filesystem::create_directories(cacheDirectory);
		}

	}

	Slang::ComPtr<slang::IGlobalSession> VulkanSlangShader::s_GlobalSession{};

	VulkanSlangShader::VulkanSlangShader(const std::string& shaderName)
	{
		m_ShaderName = shaderName;

		Utils::CreateCacheDirectoryIfRequired();

		CompileOrGetSlangBinaries();
		ReflectShaderData();
	}

	void VulkanSlangShader::CreateGlobalSession()
	{
		if (s_GlobalSession)
			return;

		// Create Global Session
		slang::createGlobalSession(s_GlobalSession.writeRef());
	}

	void VulkanSlangShader::Reload()
	{
	}

	// For GLSL to HLSL/Slang Mapping
	// https://anteru.net/blog/2016/mapping-between-HLSL-and-GLSL/
	void VulkanSlangShader::CompileOrGetSlangBinaries()
	{
		Timer timer("Slang Shader Compilation");

		const char* shaderPaths[] = { "cache/slang", "shaders", "shaders/Utils" };

		// Compiler Options
		std::array<slang::CompilerOptionEntry, 1> options = {
			{
				slang::CompilerOptionName::UseUpToDateBinaryModule,
				{ slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr }
			}
		};

		// Target Description
		slang::TargetDesc targetDesc{};
		targetDesc.format = SLANG_SPIRV;
		targetDesc.profile = s_GlobalSession->findProfile("spirv_1_4");

		// Session Description
		slang::SessionDesc sessionDesc{};
		sessionDesc.searchPaths = shaderPaths;
		sessionDesc.searchPathCount = 3;
		sessionDesc.targets = &targetDesc;
		sessionDesc.targetCount = 1;
#if 0
		sessionDesc.preprocessorMacros = preprocessorMacroDesc.data();
		sessionDesc.preprocessorMacroCount = (uint32_t)preprocessorMacroDesc.size();
#endif
		sessionDesc.compilerOptionEntries = options.data();
		sessionDesc.compilerOptionEntryCount = options.size();

		// All Shader Types
		constexpr std::array<ShaderType, 6> shaderTypes = {
			ShaderType::Vertex,
			ShaderType::Fragment,
			ShaderType::TessellationControl,
			ShaderType::TessellationEvaluation,
			ShaderType::Geometry,
			ShaderType::Compute
		};

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();
		const std::string slangModuleExtension = ".slang-module";

		Slang::ComPtr<slang::IBlob> moduleDiagnostics{}; // Diagnostics Blob(For Error Messaging)
		std::vector<slang::IComponentType*> componentsData{}; // Store Modules and Entry Points
		componentsData.reserve(6);

		// Create Session
		if (!m_SlangSession)
			s_GlobalSession->createSession(sessionDesc, m_SlangSession.writeRef());

		m_SlangModule = m_SlangSession->loadModule(m_ShaderName.c_str(), moduleDiagnostics.writeRef());

		// Cache Loaded Modules to disk(if module doesn't exist)
		for (int i = 0; i < m_SlangSession->getLoadedModuleCount(); ++i)
		{
			auto module = m_SlangSession->getLoadedModule(i);
			auto name = module->getName();

			std::filesystem::path cachedPath = cacheDirectory / (name + slangModuleExtension);
			if (!std::filesystem::exists(cachedPath))
			{
				Slang::ComPtr<slang::IBlob> moduleBlob{};
				module->serialize(moduleBlob.writeRef());

				std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
				if (out.is_open())
				{
					out.write((char*)moduleBlob->getBufferPointer(), moduleBlob->getBufferSize());
					out.flush();
					out.close();
				}
			}
		}

		if (m_SlangModule)
			componentsData.push_back(m_SlangModule);
		else
		{
			VK_CORE_ASSERT(!moduleDiagnostics, "{0} Shader Compilation Error: {1}", m_ShaderName, (const char*)moduleDiagnostics->getBufferPointer());
			moduleDiagnostics->release();
		}

		for (auto shaderType : shaderTypes)
		{
			Slang::ComPtr<slang::IEntryPoint> entryPoint{};
			m_SlangModule->findEntryPointByName(Utils::GetSlangEntryPointFromType(shaderType), entryPoint.writeRef());

			if (entryPoint)
				componentsData.emplace_back(entryPoint);
		}

		// Compose Program
		Slang::ComPtr<slang::IComponentType> composedProgram{};
		Slang::ComPtr<slang::IBlob> programDiagnostics{}; // Diagnostics Blob(For Error Messaging)

		SlangResult result = m_SlangSession->createCompositeComponentType(
			componentsData.data(), componentsData.size(),
			composedProgram.writeRef(), programDiagnostics.writeRef());

		VK_CORE_ASSERT(result == SLANG_OK, "{0} Shader Compose Error: {1}", m_ShaderName, (const char*)programDiagnostics->getBufferPointer());

		// Link Program
		Slang::ComPtr<slang::IComponentType> linkedProgram{};
		result = composedProgram->link(linkedProgram.writeRef(), programDiagnostics.writeRef());

		VK_CORE_ASSERT(result == SLANG_OK, "{0} Shader Linking Error: {1}", m_ShaderName, (const char*)programDiagnostics->getBufferPointer());

		// Shader Reflection
		auto programLayout = linkedProgram->getLayout(0);
		for (int i = 0; i < programLayout->getEntryPointCount(); ++i)
		{
			auto entryPointRefl = programLayout->getEntryPointByIndex(i);
			auto shaderType = Utils::SlangShaderTypeToGLShaderType(entryPointRefl->getStage());

			// Get Target Code(SPIR-V Code)
			Slang::ComPtr<slang::IBlob> spirvCode{};
			Slang::ComPtr<slang::IBlob> diagnosticsBlob{};
			SlangResult result = linkedProgram->getEntryPointCode(i, 0, spirvCode.writeRef(), diagnosticsBlob.writeRef());

			VK_CORE_ASSERT(result == SLANG_OK, "Failed to find SPIR-V Code: {}", (const char*)diagnosticsBlob->getBufferPointer());

			auto bufferPtr = reinterpret_cast<const uint32_t*>(spirvCode->getBufferPointer());
			uint32_t spirvCodeSize = spirvCode->getBufferSize() / sizeof(uint32_t);

			// Copy SPIR-V Code to Vulkan Binary
			m_VulkanSPIRV[(uint32_t)shaderType] = { &bufferPtr[0], &bufferPtr[spirvCodeSize] };
		}
	}

	void VulkanSlangShader::ReflectShaderData()
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

}
