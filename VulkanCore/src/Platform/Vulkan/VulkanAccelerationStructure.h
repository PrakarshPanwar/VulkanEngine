#pragma once
#include "VulkanContext.h"
#include "VulkanCore/Renderer/AccelerationStructure.h"

namespace VulkanCore {

	struct VulkanAccelerationStructureInfo
	{
		VkBuffer Buffer = nullptr;
		VmaAllocation MemoryAlloc = nullptr;
		VkAccelerationStructureKHR Handle = nullptr;
		VkDeviceAddress DeviceAddress = 0;
	};

	class VulkanAccelerationStructure : public AccelerationStructure
	{
	public:
		VulkanAccelerationStructure();
		~VulkanAccelerationStructure();

		void BuildTopLevelAccelerationStructure() override;
		void BuildBottomLevelAccelerationStructure() override;
		void SubmitMesh(std::shared_ptr<Mesh> mesh, std::shared_ptr<VertexBuffer> transformBuffer, uint32_t transformOffset) override;

		inline const VkWriteDescriptorSetAccelerationStructureKHR& GetDescriptorAccelerationStructureInfo() const
		{
			return m_DescriptorAccelerationStructureInfo;
		}
	private:
		VulkanAccelerationStructureInfo m_TopLevelAccelerationStructureInfo{};
		VulkanAccelerationStructureInfo m_BottomLevelAccelerationStructureInfo{};

		std::vector<VkAccelerationStructureGeometryKHR> m_AccelerationStructuresGeometries;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> m_AccelerationStructuresBuildRanges;
		std::vector<uint32_t> m_GeometryCount;

		VkWriteDescriptorSetAccelerationStructureKHR m_DescriptorAccelerationStructureInfo{};
	};

}
