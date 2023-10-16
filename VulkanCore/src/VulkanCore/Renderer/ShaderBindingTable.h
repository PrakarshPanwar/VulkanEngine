#pragma once
#include "VulkanCore/Renderer/Shader.h"

namespace VulkanCore {

	class ShaderBindingTable
	{
	public:
		virtual std::shared_ptr<Shader> GetShader() const = 0;
	};

}
