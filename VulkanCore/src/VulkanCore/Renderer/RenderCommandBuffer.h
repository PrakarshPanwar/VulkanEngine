#pragma once
#include "Resource.h"

namespace VulkanCore {

	class RenderCommandBuffer : public Resource
	{
	public:
		virtual void Begin() = 0;
		virtual void End() = 0;
	};

}
