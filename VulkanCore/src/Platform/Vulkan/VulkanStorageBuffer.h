#pragma once
#include "VulkanContext.h"

namespace VulkanCore {

	class VulkanStorageBuffer
	{
	public:
		VulkanStorageBuffer() = default;
		VulkanStorageBuffer(uint32_t size);

		~VulkanStorageBuffer();

		void WriteandFlushBuffer(void* data, VkDeviceSize offset = 0);

		inline const VkDescriptorBufferInfo& GetDescriptorBufferInfo() const { return m_DescriptorBufferInfo; }
	private:
		VkBuffer m_VulkanBuffer = nullptr;
		VmaAllocation m_MemoryAllocation;
		uint8_t* m_dstData = nullptr;
		uint32_t m_Size;
		VkDescriptorBufferInfo m_DescriptorBufferInfo{};
	};

}
