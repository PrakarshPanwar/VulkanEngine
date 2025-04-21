#pragma once
#include "Jolt/Jolt.h"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Physics/PhysicsSettings.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Body/BodyActivationListener.h"

#include "VulkanCore/Core/Components.h"

namespace VulkanCore {

	namespace JoltLayers {

		static constexpr JPH::ObjectLayer NON_MOVING = 0;
		static constexpr JPH::ObjectLayer MOVING = 1;
		static constexpr JPH::ObjectLayer NUM_LAYERS = 2;

	}

	namespace JoltBroadPhaseLayers {
		
		static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
		static constexpr JPH::BroadPhaseLayer MOVING(1);
		static constexpr uint32_t NUM_LAYERS(2);

	}

	namespace Utils {

		std::tuple<JPH::EMotionType, JPH::EActivation, JPH::ObjectLayer, JPH::BroadPhaseLayer> GetBodyType(Rigidbody3DComponent::BodyType bodyType);

	}

	// Class that determines if two Object layers can collide
	class MObjectLayerPairFilter : public JPH::ObjectLayerPairFilter
	{
	public:
		bool ShouldCollide(JPH::ObjectLayer objLayerA, JPH::ObjectLayer objLayerB) const override;
	};

	// BroadPhaseLayerInterface Implementation
	// This defines a Mapping between Object and Broadphase layers
	class MBroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface
	{
	public:
		MBroadPhaseLayerInterface();

		uint32_t GetNumBroadPhaseLayers() const override;
		JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;
	private:
		JPH::BroadPhaseLayer m_ObjectToBroadPhase[JoltLayers::NUM_LAYERS];
	};

	// Class that determines if an Object layer can collide with a Broadphase layer
	class MObjectVsBroadPhaseLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		bool ShouldCollide(JPH::ObjectLayer objLayer, JPH::BroadPhaseLayer bpLayer) const override;
	};

}
