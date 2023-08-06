#pragma once
#include "Resource.h"
#include "VulkanCore/Renderer/Shader.h"

namespace VulkanCore {

	class RayTracingPipeline : public Resource
	{
	public:
		virtual void CreateShaderBindingTable() = 0;
		virtual void ReloadPipeline() = 0;
		virtual std::shared_ptr<Shader> GetShader() const = 0;
	};

}
