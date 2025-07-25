#include "vulkanpch.h"
#include "PhysicsDebugRenderer.h"

#include "Platform/JoltPhysics/JoltDebugRenderer.h"

namespace VulkanCore {

	std::shared_ptr<PhysicsDebugRenderer> PhysicsDebugRenderer::Create()
	{
		return std::make_shared<JoltDebugRenderer>();
	}

}
