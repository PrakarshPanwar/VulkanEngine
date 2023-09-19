#pragma once
#include "VulkanContext.h"
#include "VulkanCore/Renderer/AccelerationStructure.h"
#include "VulkanCore/Mesh/Mesh.h"
#include "VulkanCore/Asset/MaterialAsset.h"

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
		void UpdateTopLevelAccelerationStructure(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer) override;
		void SubmitMeshDrawData(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const std::vector<TransformData>& transformData, uint32_t submeshIndex, uint32_t instanceCount) override;
		void UpdateInstancesData(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const std::vector<TransformData>& transformData, uint32_t submeshIndex) override;

		inline const VkWriteDescriptorSetAccelerationStructureKHR& GetDescriptorAccelerationStructureInfo() const
		{
			return m_DescriptorAccelerationStructureInfo;
		}
	private:
		void Release();
	private:
		struct BLASInput
		{
			std::string DebugName;
			VkAccelerationStructureGeometryKHR GeometryData{};
			VkAccelerationStructureBuildRangeInfoKHR BuildRangeInfo{};
			VulkanAccelerationStructureInfo BLASInfo{};
			std::vector<VkAccelerationStructureInstanceKHR> InstanceData{};
			uint32_t InstanceCount = 0;
		};
	private:
		VulkanAccelerationStructureInfo m_TLASInfo{};
		uint32_t m_InstanceIndex = 0;
		VkBuffer m_InstanceBuffer, m_ScratchBuffer;
		VkDeviceAddress m_InstanceDeviceAddress, m_ScratchDeviceAddress;
		VmaAllocation m_InstanceBufferAlloc, m_ScratchBufferAlloc;
		uint8_t* m_InstanceBufferDstData = nullptr;

		std::map<MeshKey, BLASInput> m_BLASInputData;

		VkWriteDescriptorSetAccelerationStructureKHR m_DescriptorAccelerationStructureInfo{};
	};

}
