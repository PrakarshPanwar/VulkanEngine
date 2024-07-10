#pragma once
#include "RenderCommandBuffer.h"

namespace VulkanCore {

	class Mesh;
	class MaterialAsset;
	struct TransformData;

	class AccelerationStructure
	{
	public:
		virtual void BuildTopLevelAccelerationStructure() = 0;
		virtual void BuildBottomLevelAccelerationStructures() = 0;
		virtual void UpdateTopLevelAccelerationStructure() = 0;
		virtual void ResetAccelerationStructures() = 0;
		virtual void SubmitMeshDrawData(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const std::vector<TransformData>& transformData, uint32_t submeshIndex, uint32_t instanceCount) = 0;
		virtual void UpdateInstancesData(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const std::vector<TransformData>& transformData, uint32_t submeshIndex) = 0;
	};

}
