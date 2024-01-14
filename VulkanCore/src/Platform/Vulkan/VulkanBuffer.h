#pragma once
#include "VulkanDevice.h"

namespace VulkanCore {

	class VulkanBuffer
	{
	public:
		VulkanBuffer(uint32_t size, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryPropertyFlags);
		~VulkanBuffer();

		void Map();
		void Unmap();

		void WriteToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		VkResult FlushBuffer(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		VkDescriptorBufferInfo DescriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

		inline VkBuffer GetVulkanBuffer() const { return m_VulkanBuffer; }
		inline void* GetMappedMemory() const { return m_MapDataPtr; }
		inline VkBufferUsageFlags GetUsageFlags() const { return m_UsageFlags; }
		inline VkMemoryPropertyFlags GetMemoryPropertyFlags() const { return m_MemoryUsageFlag; }
		inline VkDeviceSize GetBufferSize() const { return m_Size; }
	private:
		static VkDeviceSize GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);
	private:
		uint8_t* m_MapDataPtr = nullptr;
		VkBuffer m_VulkanBuffer = VK_NULL_HANDLE;
		VkDeviceMemory m_Memory = VK_NULL_HANDLE;
		VmaAllocation m_MemoryAllocation;

		uint32_t m_Size;
		VkBufferUsageFlags m_UsageFlags;
		VmaMemoryUsage m_MemoryUsageFlag;
	};

}