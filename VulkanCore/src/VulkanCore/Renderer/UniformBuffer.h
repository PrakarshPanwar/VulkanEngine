#pragma once
#include "Resource.h"

namespace VulkanCore {

	class UniformBuffer : public Resource
	{
	public:
		virtual void WriteAndFlushBuffer(void* data, uint32_t offset = 0) = 0;
	};

}
