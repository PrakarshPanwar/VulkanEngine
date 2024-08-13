#pragma once
#include "Resource.h"

namespace VulkanCore {

	class StorageBuffer : public Resource
	{
	public:
		virtual void WriteData(void* data, uint32_t offset = 0) = 0;
	};

}
