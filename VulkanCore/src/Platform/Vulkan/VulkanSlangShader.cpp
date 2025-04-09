#include "vulkanpch.h"
#include "VulkanSlangShader.h"

#include "VulkanCore/Core/Core.h"

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

		static const char* GetEntryPointFromType(ShaderType stage)
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

	}

	Slang::ComPtr<slang::IGlobalSession> VulkanSlangShader::s_GlobalSession{};

	VulkanSlangShader::VulkanSlangShader(const std::string& shaderName)
	{
		m_ShaderName = shaderName;

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

	void VulkanSlangShader::CompileOrGetSlangBinaries()
	{
		const char* shaderPaths[] = { "shaders", "shaders/Utils" };

		// Compiler Options
		std::array<slang::CompilerOptionEntry, 1> options = {
			{
				slang::CompilerOptionName::EmitSpirvDirectly,
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
		sessionDesc.searchPathCount = 2;
		sessionDesc.targets = &targetDesc;
		sessionDesc.targetCount = 1;
#if 0
		sessionDesc.preprocessorMacros = preprocessorMacroDesc.data();
		sessionDesc.preprocessorMacroCount = (uint32_t)preprocessorMacroDesc.size();
#endif
		sessionDesc.compilerOptionEntries = options.data();
		sessionDesc.compilerOptionEntryCount = options.size();

		// Create Session
		Slang::ComPtr<slang::ISession> session{};
		s_GlobalSession->createSession(sessionDesc, session.writeRef());

		std::vector<slang::IComponentType*> componentTypes{}; // Store Modules and Entry Points

		// Create Slang Module
		Slang::ComPtr<slang::IModule> slangModule{};
		{
			Slang::ComPtr<slang::IBlob> diagnosticsBlob{};
			slangModule = session->loadModule(m_ShaderName.c_str(), diagnosticsBlob.writeRef());

			if (slangModule)
				componentTypes.push_back(slangModule);
			else
				VK_CORE_ASSERT(!diagnosticsBlob, "Compilation Error: {}", (const char*)diagnosticsBlob->getBufferPointer());
		}

		// Query Entry Points
		constexpr std::array<ShaderType, 6> shaderTypes = {
			ShaderType::Vertex,
			ShaderType::Fragment,
			ShaderType::TessellationControl,
			ShaderType::TessellationEvaluation,
			ShaderType::Geometry,
			ShaderType::Compute
		};

		for (auto shaderType : shaderTypes)
		{
			Slang::ComPtr<slang::IEntryPoint> entryPoint{};
			Slang::ComPtr<slang::IBlob> diagnosticsBlob{};
			slangModule->findEntryPointByName(Utils::GetEntryPointFromType(shaderType), entryPoint.writeRef());

			if (entryPoint)
				componentTypes.push_back(entryPoint);
			else
				VK_CORE_ASSERT(!diagnosticsBlob, "Entry Point not found: {}", (const char*)diagnosticsBlob->getBufferPointer());
		}

		// Compose Program
		Slang::ComPtr<slang::IComponentType> composedProgram{};
		{
			Slang::ComPtr<slang::IBlob> diagnosticsBlob{};
			SlangResult result = session->createCompositeComponentType(
				componentTypes.data(), componentTypes.size(),
				composedProgram.writeRef(), diagnosticsBlob.writeRef());

			SLANG_RETURN_VOID_ON_FAIL(result);
		}

		// Link Program
		Slang::ComPtr<slang::IComponentType> linkedProgram{};
		{
			Slang::ComPtr<slang::IBlob> diagnosticsBlob{};
			SlangResult result = composedProgram->link(linkedProgram.writeRef(), diagnosticsBlob.writeRef());

			SLANG_RETURN_VOID_ON_FAIL(result);
		}

		for (uint32_t i = 0; i < componentTypes.size() - 1; ++i)
		{
			// Get Target Code(SPIR-V Code)
			Slang::ComPtr<slang::IBlob> spirvCode{};
			Slang::ComPtr<slang::IBlob> diagnosticsBlob{};
			SlangResult result = linkedProgram->getEntryPointCode(i, 0, spirvCode.writeRef(), diagnosticsBlob.writeRef());

			VK_CORE_ASSERT(result == SLANG_OK, "Failed to find SPIR-V Code: {}", (const char*)diagnosticsBlob->getBufferPointer());

			auto bufferPtr = reinterpret_cast<const uint32_t*>(spirvCode->getBufferPointer());
			auto bufferSize = spirvCode->getBufferSize();

			// Copy SPIR-V Code to Vulkan Binary
			std::vector<uint32_t> spirvBinary{};
			uint32_t spirvCodeSize = spirvCode->getBufferSize() / sizeof(uint32_t);
			spirvBinary.insert(spirvBinary.end(), &bufferPtr[0], &bufferPtr[spirvCodeSize]);

			m_VulkanSPIRV[(uint32_t)shaderTypes[i]] = spirvBinary;
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
