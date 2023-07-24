#pragma once

namespace VulkanCore {

	class AccelerationStructure
	{
	public:
		virtual void BuildTopLevelAccelerationStructure() = 0;
		virtual void BuildBottomLevelAccelerationStructure() = 0;
	};

}
