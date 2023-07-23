#pragma once
#include "VulkanCore/Mesh/Mesh.h"

namespace VulkanCore {

	class AccelerationStructure
	{
	public:
		virtual void BuildTopLevelAccelerationStructure() = 0;
		virtual void BuildBottomLevelAccelerationStructure() = 0;
		virtual void SubmitMesh(std::shared_ptr<Mesh> mesh, std::shared_ptr<VertexBuffer> transformBuffer, uint32_t transformOffset) = 0;
	};

}
