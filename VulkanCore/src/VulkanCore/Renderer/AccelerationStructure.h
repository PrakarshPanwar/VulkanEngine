#pragma once

namespace VulkanCore {

	class Mesh;
	struct TransformData;
	struct MeshKey;

	class AccelerationStructure
	{
	public:
		virtual void BuildTopLevelAccelerationStructure() = 0;
		virtual void BuildBottomLevelAccelerationStructures() = 0;
		virtual void SubmitMeshDrawData(const MeshKey& meshKey, const std::vector<TransformData>& transformData, uint32_t instanceCount) = 0;
	};

}
