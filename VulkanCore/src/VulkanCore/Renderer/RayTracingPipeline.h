#pragma once
#include "Resource.h"

namespace VulkanCore {

	class RayTracingPipeline : public Resource
	{
	public:
		virtual void CreateShaderBindingTable() = 0;
	};

}
