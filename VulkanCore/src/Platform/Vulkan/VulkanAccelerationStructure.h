#pragma once
#include "VulkanContext.h"
#include "VulkanCore/Renderer/AccelerationStructure.h"
#include "VulkanCore/Mesh/Mesh.h"

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
		void BuildBottomLevelAccelerationStructures() override;
		void SubmitMeshDrawData(const MeshKey& meshKey, const std::vector<TransformData>& transformData, uint32_t instanceCount) override;

		inline const VkWriteDescriptorSetAccelerationStructureKHR& GetDescriptorAccelerationStructureInfo() const
		{
			return m_DescriptorAccelerationStructureInfo;
		}
	private:
		struct BLASInput
		{
			VkAccelerationStructureGeometryKHR GeometryData{};
			VkAccelerationStructureBuildRangeInfoKHR BuildRangeInfo{};
			VulkanAccelerationStructureInfo BLASInfo{};
			std::vector<VkAccelerationStructureInstanceKHR> InstanceData{};
			uint32_t InstanceCount = 0;
		};
	private:
		VulkanAccelerationStructureInfo m_TLASInfo{};
		VkBuffer m_InstanceBuffer = nullptr;
		VmaAllocation m_InstanceBufferAlloc = nullptr;

		std::map<MeshKey, BLASInput> m_BLASInputData;

		VkWriteDescriptorSetAccelerationStructureKHR m_DescriptorAccelerationStructureInfo{};
	};

}