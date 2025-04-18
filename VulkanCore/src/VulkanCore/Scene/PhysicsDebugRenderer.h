#pragma once
#include "VulkanCore/Renderer/RenderCommandBuffer.h"

namespace VulkanCore {

	class PhysicsDebugRenderer
	{
	public:
		virtual void FlushData() = 0;
		virtual void ClearData() = 0;
		virtual void Draw(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer) = 0;

		static std::shared_ptr<PhysicsDebugRenderer> Create();
	};

}
