#pragma once
#include "Resource.h"
#include "Shader.h"

namespace VulkanCore {

	class ComputePipeline : public Resource
	{
	public:
		virtual std::shared_ptr<Shader> GetShader() const = 0;
	};

}
