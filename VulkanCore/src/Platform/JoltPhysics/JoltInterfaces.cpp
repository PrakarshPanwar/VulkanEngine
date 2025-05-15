#include "vulkanpch.h"
#include "JoltInterfaces.h"

namespace VulkanCore {

	namespace Utils {

		std::tuple<JPH::EMotionType, JPH::EActivation, JPH::ObjectLayer, JPH::BroadPhaseLayer> GetBodyType(Rigidbody3DComponent::BodyType bodyType)
		{
			switch (bodyType)
			{
			case Rigidbody3DComponent::BodyType::Static:	return { JPH::EMotionType::Static, JPH::EActivation::DontActivate, JoltLayers::NON_MOVING, JoltBroadPhaseLayers::NON_MOVING };
			case Rigidbody3DComponent::BodyType::Dynamic:	return { JPH::EMotionType::Dynamic, JPH::EActivation::Activate, JoltLayers::MOVING, JoltBroadPhaseLayers::MOVING };
			case Rigidbody3DComponent::BodyType::Kinematic: return { JPH::EMotionType::Kinematic, JPH::EActivation::Activate, JoltLayers::MOVING, JoltBroadPhaseLayers::MOVING };
			default:
				VK_CORE_ASSERT(false, "Unsupported Rigid Body!");
				return { JPH::EMotionType::Static, JPH::EActivation::DontActivate, JoltLayers::NON_MOVING, JoltBroadPhaseLayers::NON_MOVING };
			};
		}

	}

	bool MObjectLayerPairFilter::ShouldCollide(JPH::ObjectLayer objLayerA, JPH::ObjectLayer objLayerB) const
	{
		switch (objLayerA)
		{
		case JoltLayers::NON_MOVING: return objLayerB == JoltLayers::MOVING; // Non-Moving only Collides with Moving
		case JoltLayers::MOVING:	 return true; // Moving Collides with Everything
		default:
			VK_CORE_ASSERT(false, "Undefined Collision Behaviour!");
			return false;
		}
	}

	MBroadPhaseLayerInterface::MBroadPhaseLayerInterface()
	{
		// Create a Mapping table from Object to Broad Phase layer
		m_ObjectToBroadPhase[JoltLayers::NON_MOVING] = JoltBroadPhaseLayers::NON_MOVING;
		m_ObjectToBroadPhase[JoltLayers::MOVING] = JoltBroadPhaseLayers::MOVING;
	}

	uint32_t MBroadPhaseLayerInterface::GetNumBroadPhaseLayers() const
	{
		return JoltBroadPhaseLayers::NUM_LAYERS;
	}

	JPH::BroadPhaseLayer MBroadPhaseLayerInterface::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const
	{
		VK_CORE_ASSERT(inLayer < JoltLayers::NUM_LAYERS, "Layer Index should be less than Total Layers!");
		return m_ObjectToBroadPhase[inLayer];
	}

	bool MObjectVsBroadPhaseLayerFilter::ShouldCollide(JPH::ObjectLayer objLayer, JPH::BroadPhaseLayer bpLayer) const
	{
		switch (objLayer)
		{
		case JoltLayers::NON_MOVING: return bpLayer == JoltBroadPhaseLayers::MOVING;
		case JoltLayers::MOVING:	 return true;
		default:
			VK_CORE_ASSERT(false, "Undefined Collision Behaviour!");
			return false;
		}
	}

}
