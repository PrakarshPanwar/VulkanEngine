#include "vulkanpch.h"
#include "JoltPhysicsWorld.h"
#include "VulkanCore/Scene/Entity.h"

// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
#define MAX_BODIES 1024

// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
#define NUM_BODY_MUTEXES 0

// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
#define MAX_BODY_PAIRS 1024

// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
// Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
#define MAX_CONTACT_CONSTRAINTS 1024

namespace VulkanCore {

	JoltPhysicsWorld::JoltPhysicsWorld()
		: m_PhysicsSystem(new JPH::PhysicsSystem), m_ObjectVsBroadPhaseLayerFilter(), m_ObjectVsObjectLayerFilter()
	{
		JPH::Factory::sInstance = new JPH::Factory();

		m_JobSystem = new JPH::JobSystemThreadPool(2, 2, 2);
	}

	JoltPhysicsWorld::~JoltPhysicsWorld()
	{
		// Unregister All Types
		JPH::UnregisterTypes();

		// Destroy Factory
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;

		if (m_JobSystem)
			delete m_JobSystem;

		if (m_PhysicsSystem)
			delete m_PhysicsSystem;
	}

	void JoltPhysicsWorld::Init(Scene* scene)
	{
		JPH::RegisterDefaultAllocator();

		m_PhysicsSystem->Init(MAX_BODIES, NUM_BODY_MUTEXES, MAX_BODY_PAIRS, MAX_CONTACT_CONSTRAINTS, m_BroadPhaseLayerInterface, m_ObjectVsBroadPhaseLayerFilter, m_ObjectVsObjectLayerFilter);
	}

	void JoltPhysicsWorld::Update(float dt)
	{
		m_PhysicsSystem->Update(dt, 1, nullptr, m_JobSystem);
	}

	void JoltPhysicsWorld::Destroy()
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterface();
	}

	void JoltPhysicsWorld::CreateBodies(Scene* scene)
	{
		JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

		auto view = scene->GetAllEntitiesWith<TransformComponent, Rigidbody3DComponent>();
		for (auto e : view)
		{
			auto [transform, rb3d] = view.get<TransformComponent, Rigidbody3DComponent>(e);
			auto [motionType, activation, objectLayer, broadPhaseLayer] = Utils::GetBodyType(rb3d.Type);

			Entity entity = { e, scene };
			if (entity.HasComponent<BoxCollider3DComponent>())
			{
				auto bc3d = entity.GetComponent<BoxCollider3DComponent>();
				JPH::BoxShapeSettings boxSettings{ JPH::Vec3(bc3d.Size.x, bc3d.Size.y, bc3d.Size.z) };
				auto shapeResult = boxSettings.Create();
				auto shapeRef = shapeResult.Get();

				// Set Body Settings
				JPH::BodyCreationSettings settings{
					shapeRef,
					JPH::Vec3(bc3d.Offset.x, bc3d.Offset.y, bc3d.Offset.z),
					JPH::Quat::sIdentity(),
					motionType,
					objectLayer
				};

				auto bodyPtr = bodyInterface.CreateBodyWithID(JPH::BodyID{ (uint32_t)e }, settings); // Create Body
				bodyInterface.AddBody(bodyPtr->GetID(), activation); // Add Body

				rb3d.RuntimeBody = bodyPtr;
			}

			if (entity.HasComponent<SphereColliderComponent>())
			{
				auto sc3d = entity.GetComponent<SphereColliderComponent>();

				JPH::SphereShapeSettings sphereSettings{ sc3d.Radius };
				auto shapeResult = sphereSettings.Create();
				auto shapeRef = shapeResult.Get();

				// Set Body Settings
				JPH::BodyCreationSettings settings{
					shapeRef,
					JPH::Vec3(sc3d.Offset.x, sc3d.Offset.y, sc3d.Offset.z),
					JPH::Quat::sIdentity(),
					motionType,
					objectLayer
				};

				auto bodyPtr = bodyInterface.CreateBodyWithID(JPH::BodyID{ (uint32_t)e }, settings); // Create Body
				bodyInterface.AddBody(bodyPtr->GetID(), activation); // Add Body

				rb3d.RuntimeBody = bodyPtr;
			}
		}
	}

#if 0
	void JoltPhysicsWorld::CreateBoxShape(Scene* scene)
	{
		JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

		auto view = scene->GetAllEntitiesWith<TransformComponent, BoxCollider3DComponent>();
		for (auto e : view)
		{
			auto [transform, bc3d] = view.get<TransformComponent, BoxCollider3DComponent>(e);

			JPH::BoxShapeSettings boxSettings{ JPH::Vec3(bc3d.Size.x, bc3d.Size.y, bc3d.Size.z) };
			auto shapeResult = boxSettings.Create();
			auto shapeRef = shapeResult.Get();

			// Set Body Settings
			JPH::BodyCreationSettings settings{
				shapeRef,
				JPH::Vec3(bc3d.Offset.x, bc3d.Offset.y, bc3d.Offset.z),
				JPH::Quat::sIdentity(),
				JPH::EMotionType::Static,
				Layers::MOVING
			};

			// Create Body
			auto bodyPtr = bodyInterface.CreateBodyWithID((uint32_t)e, settings);
			bodyInterface.AddBody(bodyPtr->GetID(), JPH::EActivation::Activate);
		}
	}

	void JoltPhysicsWorld::CreateSphereShape(Scene* scene)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}
#endif
}
