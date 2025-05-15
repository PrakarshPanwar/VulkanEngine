#pragma once
#include "VulkanCore/Scene/PhysicsWorld.h"
#include "JoltInterfaces.h"

namespace VulkanCore {

	class JoltPhysicsWorld : public PhysicsWorld
	{
	public:
		JoltPhysicsWorld();
		~JoltPhysicsWorld();

		void Init(Scene* scene) override;
		void Update(Timestep ts, Scene* scene) override;
		void DestroySystem() override;

		void CreateBodies(Scene* scene) override;
		void RemoveAndDestroyBodies(Scene* scene) override;
		void DrawPhysicsBodies(std::shared_ptr<PhysicsDebugRenderer> debugRenderer) override;

		bool IsValid() override;
	//protected:
		//void CreateBoxShape(Scene* scene) override;
		//void CreateSphereShape(Scene* scene) override;
	private:
		JPH::PhysicsSystem* m_PhysicsSystem = nullptr;
		JPH::TempAllocator* m_TempAllocator = nullptr;
		JPH::JobSystemThreadPool* m_JobSystem = nullptr;

		// Create mapping table from object layer to broadphase layer
		// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
		// Also have a look at BroadPhaseLayerInterfaceTable or BroadPhaseLayerInterfaceMask for a simpler interface.
		MBroadPhaseLayerInterface m_BroadPhaseLayerInterface;

		// Create class that filters object vs broadphase layers
		// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
		// Also have a look at ObjectVsBroadPhaseLayerFilterTable or ObjectVsBroadPhaseLayerFilterMask for a simpler interface.
		MObjectVsBroadPhaseLayerFilter m_ObjectVsBroadPhaseLayerFilter{};

		// Create class that filters object vs object layers
		// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
		// Also have a look at ObjectLayerPairFilterTable or ObjectLayerPairFilterMask for a simpler interface.
		MObjectLayerPairFilter m_ObjectVsObjectLayerFilter{};
	};

}
