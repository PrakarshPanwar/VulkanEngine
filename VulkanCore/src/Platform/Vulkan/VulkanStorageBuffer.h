#pragma once
#include "VulkanContext.h"
#include "VulkanCore/Renderer/StorageBuffer.h"

namespace VulkanCore {

	class VulkanStorageBuffer : public StorageBuffer
	{
	public:
		VulkanStorageBuffer() = default;
		VulkanStorageBuffer(uint32_t size);
		~VulkanStorageBuffer();

		void WriteData(void* data, uint32_t offset) override;

		const VkDescriptorBufferInfo& GetDescriptorBufferInfo() const { return m_DescriptorBufferInfo; }
	private:
		VkBuffer m_VulkanBuffer = nullptr;
		VmaAllocation m_MemoryAllocation;
		uint8_t* m_MapDataPtr = nullptr;
		uint32_t m_Size;
		VkDescriptorBufferInfo m_DescriptorBufferInfo{};
	};

}
