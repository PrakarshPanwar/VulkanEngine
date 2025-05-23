#pragma once
#include "VulkanCore/Core/Timestep.h"
#include "VulkanCore/Core/Components.h"
#include "PhysicsDebugRenderer.h"

namespace VulkanCore {

	class Scene;

	class PhysicsWorld
	{
	public:
		PhysicsWorld() = default;
		~PhysicsWorld() = default;

		virtual void Init(Scene* scene) = 0;
		virtual void Update(Timestep ts, Scene* scene) = 0;
		virtual void DestroySystem() = 0;

		virtual void CreateBodies(Scene* scene) = 0;
		virtual void RemoveAndDestroyBodies(Scene* scene) = 0;
		virtual void DrawPhysicsBodies(std::shared_ptr<PhysicsDebugRenderer> debugRenderer) = 0;

		//	To check if System is initialized
		virtual bool IsValid() = 0;

		static std::unique_ptr<PhysicsWorld> Create();
// 	protected:
// 		virtual void CreateBoxShape(Scene* scene) = 0;
// 		virtual void CreateSphereShape(Scene* scene) = 0;
	};

}
