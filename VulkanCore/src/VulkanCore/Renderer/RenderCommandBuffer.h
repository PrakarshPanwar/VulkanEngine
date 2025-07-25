#pragma once
#include "Resource.h"

namespace VulkanCore {

	class RenderCommandBuffer : public Resource
	{
	public:
		virtual void Begin() const = 0;
		virtual void End() const = 0;
	};

}
