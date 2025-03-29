#include "vulkanpch.h"
#include "PhysicsWorld.h"
#include "Platform/JoltPhysics/JoltPhysicsWorld.h"

namespace VulkanCore {

	std::unique_ptr<PhysicsWorld> PhysicsWorld::Create()
	{
		return std::make_unique<JoltPhysicsWorld>();
	}

}
