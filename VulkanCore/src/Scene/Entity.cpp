#include "vulkanpch.h"
#include "Entity.h"

namespace VulkanCore {

	Entity::Entity(entt::entity handle, Scene* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}

}