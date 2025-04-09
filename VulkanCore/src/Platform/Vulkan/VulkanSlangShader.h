#pragma once
#include "VulkanShader.h"
#include "VulkanDescriptor.h"

#include <slang/slang.h>
#include <slang/slang-com-ptr.h>

namespace VulkanCore {

	class VulkanSlangShader : public VulkanShader
	{
	public:
		VulkanSlangShader() = default;
		VulkanSlangShader(const std::string& shaderName);

		static void CreateGlobalSession();

		void Reload() override;
	private:
		void CompileOrGetSlangBinaries();
		void ReflectShaderData();

		static Slang::ComPtr<slang::IGlobalSession> s_GlobalSession;
	};

}
