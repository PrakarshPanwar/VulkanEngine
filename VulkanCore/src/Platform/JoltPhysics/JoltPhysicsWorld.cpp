#include "vulkanpch.h"
#include "JoltPhysicsWorld.h"
#include "VulkanCore/Scene/Entity.h"
#include "VulkanCore/Asset/AssetManager.h"
#include "VulkanCore/Asset/Asset.h"

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
	{
		// Register allocation hook. In this example we'll just let Jolt use malloc / free but you can override these if you want (see Memory.h).
		// This needs to be done before any other Jolt function is called.
		JPH::RegisterDefaultAllocator();

		// Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
		// It is not directly used currently but still required.
		JPH::Factory::sInstance = new JPH::Factory();

		// Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
		// If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
		// If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
		JPH::RegisterTypes();

		m_TempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024); // 10 MB

		// We need a job system that will execute physics jobs on multiple threads
		// Typically you would implement the JobSystem interface yourself and let Jolt Physics run on top
		// of your own job scheduler. JobSystemThreadPool is an example implementation.
		m_JobSystem = new JPH::JobSystemThreadPool(2048, 8, 2);
	}

	JoltPhysicsWorld::~JoltPhysicsWorld()
	{
		// Unregister All Types
		JPH::UnregisterTypes();

		// Destroy Factory
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;

		if (m_JobSystem)
		{
			delete m_JobSystem;
			m_JobSystem = nullptr;
		}

		if (m_TempAllocator)
		{
			delete m_TempAllocator;
			m_TempAllocator = nullptr;
		}

		if (m_PhysicsSystem)
		{
			delete m_PhysicsSystem;
			m_PhysicsSystem = nullptr;
		}
	}

	void JoltPhysicsWorld::Init(Scene* scene)
	{
		// Create the Physics System
		JPH::PhysicsSettings jphSettings{};
		jphSettings.mNumVelocitySteps = 6;
		jphSettings.mNumPositionSteps = 2;

		m_PhysicsSystem = new JPH::PhysicsSystem;
		m_PhysicsSystem->Init(MAX_BODIES, NUM_BODY_MUTEXES, MAX_BODY_PAIRS, MAX_CONTACT_CONSTRAINTS, m_BroadPhaseLayerInterface, m_ObjectVsBroadPhaseLayerFilter, m_ObjectVsObjectLayerFilter);
		m_PhysicsSystem->SetPhysicsSettings(jphSettings);
		m_PhysicsSystem->SetGravity(JPH::Vec3(0, -9.81f, 0)); // Default Gravity

		VK_CORE_INFO("Initialized Physics System!");
	}

	void JoltPhysicsWorld::Update(Scene* scene)
	{
		JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

		constexpr uint32_t collisionSteps = 1;
		constexpr float deltaTime = 1.0f / 60.0f; // It's kept at 60 FPS shouldn't be variable to Timestep

		m_PhysicsSystem->Update(deltaTime, collisionSteps, m_TempAllocator, m_JobSystem);

		// Update Transform Components
		auto view = scene->GetAllEntitiesWith<TransformComponent, Rigidbody3DComponent>();
		for (auto ent : view)
		{
			auto [transform, rb3d] = view.get<TransformComponent, Rigidbody3DComponent>(ent);
			auto bodyID = JPH::BodyID{ (uint32_t)ent };

			// Get Body Position and Rotation
			JPH::RVec3 bodyPosition = bodyInterface.GetPosition(bodyID);
			JPH::Quat bodyQuaternion = bodyInterface.GetRotation(bodyID);
			JPH::Vec3 bodyRotation = bodyQuaternion.GetEulerAngles();

			// Update Transform
			transform.Translation = { bodyPosition.GetX(), bodyPosition.GetY(), bodyPosition.GetZ() };
			transform.Rotation = { bodyRotation.GetX(), bodyRotation.GetY(), bodyRotation.GetZ() };
		}
	}

	void JoltPhysicsWorld::DestroySystem()
	{
		if (m_PhysicsSystem)
		{
			delete m_PhysicsSystem;
			m_PhysicsSystem = nullptr;

			VK_CORE_INFO("Destroyed Physics System!");
		}
	}

	void JoltPhysicsWorld::CreateBodies(Scene* scene)
	{
		JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

		auto view = scene->GetAllEntitiesWith<TransformComponent, Rigidbody3DComponent>();
		for (auto ent : view)
		{
			auto [transform, rb3d] = view.get<TransformComponent, Rigidbody3DComponent>(ent);
			auto [motionType, activation, objectLayer, broadPhaseLayer] = Utils::GetBodyType(rb3d.Type);

			Entity entity = { ent, scene };
			if (entity.HasComponent<BoxCollider3DComponent>())
			{
				auto& bc3d = entity.GetComponent<BoxCollider3DComponent>();

				JPH::BoxShapeSettings boxSettings{ JPH::Vec3(bc3d.Size.x, bc3d.Size.y, bc3d.Size.z) };
				boxSettings.SetDensity(bc3d.Density);

				auto shapeResult = boxSettings.Create();
				const auto& shapeRef = shapeResult.Get();

				// Obtain Transforms
				glm::quat bdQuat(transform.Rotation);
				auto bodyTransform = JPH::RVec3(transform.Translation.x, transform.Translation.y, transform.Translation.z);
				auto bodyQuaternion = JPH::Quat(bdQuat.x, bdQuat.y, bdQuat.z, bdQuat.w);

				// Set Body Settings
				JPH::BodyCreationSettings settings{
					shapeRef,
					bodyTransform,
					bodyQuaternion,
					motionType,
					objectLayer
				};

				auto bodyPtr = bodyInterface.CreateBodyWithID(JPH::BodyID{ (uint32_t)ent }, settings); // Create Body with Entity ID
				bodyPtr->SetFriction(bc3d.Friction);
				bodyPtr->SetRestitution(bc3d.Restitution);

				bodyInterface.AddBody(bodyPtr->GetID(), activation); // Add Body
			}

			if (entity.HasComponent<SphereColliderComponent>())
			{
				auto& sc3d = entity.GetComponent<SphereColliderComponent>();

				JPH::SphereShapeSettings sphereSettings{ sc3d.Radius };
				sphereSettings.SetDensity(sc3d.Density);

				auto shapeResult = sphereSettings.Create();
				const auto& shapeRef = shapeResult.Get();

				// Set Body Settings
				JPH::BodyCreationSettings settings{
					shapeRef,
					JPH::RVec3(transform.Translation.x, transform.Translation.y, transform.Translation.z),
					JPH::Quat::sIdentity(),
					motionType,
					objectLayer
				};

				auto bodyPtr = bodyInterface.CreateBodyWithID(JPH::BodyID{ (uint32_t)ent }, settings); // Create Body with Entity ID
				bodyPtr->SetFriction(sc3d.Friction);
				bodyPtr->SetRestitution(sc3d.Restitution);

				bodyInterface.AddBody(bodyPtr->GetID(), activation); // Add Body
			}

			if (entity.HasAllComponent<MeshComponent, MeshColliderComponent>())
			{
				auto& mc3d = entity.GetComponent<MeshColliderComponent>();
				auto& mc = entity.GetComponent<MeshComponent>();

				auto meshAsset = AssetManager::GetAsset<Mesh>(mc.MeshHandle);
				auto& meshVertexData = meshAsset->GetMeshVertices();
				auto& meshIndexData = meshAsset->GetMeshIndices();

				JPH::VertexList vertices{};
				JPH::IndexedTriangleList triangleList{};
				for (int i = 0; i < meshVertexData.size(); ++i)
				{
					vertices.emplace_back(
						meshVertexData[i].Position.x * transform.Scale.x,
						meshVertexData[i].Position.y * transform.Scale.y,
						meshVertexData[i].Position.z * transform.Scale.z);
				}

				for (int i = 0; i < meshIndexData.size(); i += 3)
				{
					triangleList.emplace_back(
						meshIndexData[i],
						meshIndexData[i + 1],
						meshIndexData[i + 2]);
				}

				JPH::MeshShapeSettings meshShapeSettings{ std::move(vertices), std::move(triangleList) };
				//meshShapeSettings.SetDensity(mc3d.Density);

				auto shapeResult = meshShapeSettings.Create();
				const auto& shapeRef = shapeResult.Get();

				// Obtain Transforms
				glm::quat bdQuat(transform.Rotation);
				auto bodyTransform = JPH::RVec3(transform.Translation.x, transform.Translation.y, transform.Translation.z);
				auto bodyQuaternion = JPH::Quat(bdQuat.x, bdQuat.y, bdQuat.z, bdQuat.w);

				// Set Body Settings
				JPH::BodyCreationSettings settings{
					shapeRef,
					bodyTransform,
					bodyQuaternion,
					motionType,
					objectLayer
				};

				auto bodyPtr = bodyInterface.CreateBodyWithID(JPH::BodyID{ (uint32_t)ent }, settings); // Create Body with Entity ID
				bodyPtr->SetFriction(mc3d.Friction);
				bodyPtr->SetRestitution(mc3d.Restitution);

				bodyInterface.AddBody(bodyPtr->GetID(), activation); // Add Body
			}
		}

		// Optimize Broad Phase Tree
		m_PhysicsSystem->OptimizeBroadPhase();
	}

	void JoltPhysicsWorld::RemoveAndDestroyBodies(Scene* scene)
	{
		JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

		auto view = scene->GetAllEntitiesWith<TransformComponent, Rigidbody3DComponent>();
		for (auto ent : view)
		{
			auto [transform, rb3d] = view.get<TransformComponent, Rigidbody3DComponent>(ent);
			auto bodyID = JPH::BodyID{ (uint32_t)ent };

			bodyInterface.RemoveBody(bodyID);  // Remove Body
			bodyInterface.DestroyBody(bodyID); // Destroy Body
		}
	}

#if 0
	void JoltPhysicsWorld::CreateBoxShape(Scene* scene)
	{
		JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

		auto view = scene->GetAllEntitiesWith<TransformComponent, BoxCollider3DComponent>();
		for (auto ent : view)
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
