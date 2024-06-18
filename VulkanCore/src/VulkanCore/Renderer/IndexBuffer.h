#pragma once
#include "Resource.h"

namespace VulkanCore {

	class IndexBuffer : public Resource
	{
		virtual void WriteData(void* data, uint32_t offset) = 0;
	};

}
