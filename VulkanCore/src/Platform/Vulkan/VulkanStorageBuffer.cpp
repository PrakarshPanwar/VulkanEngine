#include "vulkanpch.h"
#include "VulkanStorageBuffer.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanAllocator.h"

namespace VulkanCore {

	VulkanStorageBuffer::VulkanStorageBuffer(uint32_t size)
		: m_Size(size)
	{
		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("StorageBuffer");

		VkBufferCreateInfo storageBufferCreateInfo{};
		storageBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		storageBufferCreateInfo.size = size;
		storageBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		storageBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		m_MemoryAllocation = allocator.AllocateBuffer(storageBufferCreateInfo, VMA_MEMORY_USAGE_AUTO, m_VulkanBuffer);

		m_MappedPtr = allocator.MapMemory<uint8_t>(m_MemoryAllocation);

		m_DescriptorBufferInfo.buffer = m_VulkanBuffer;
		m_DescriptorBufferInfo.range = m_Size;
		m_DescriptorBufferInfo.offset = 0;
	}

	VulkanStorageBuffer::~VulkanStorageBuffer()
	{
		VulkanAllocator allocator("StorageBuffer");

		allocator.UnmapMemory(m_MemoryAllocation);
		allocator.DestroyBuffer(m_VulkanBuffer, m_MemoryAllocation);
	}

	void VulkanStorageBuffer::WriteAndFlushBuffer(void* data, uint32_t offset)
	{
		memcpy(m_MappedPtr, data, m_Size);
		vmaFlushAllocation(VulkanContext::GetVulkanMemoryAllocator(), m_MemoryAllocation, (VkDeviceSize)offset, (VkDeviceSize)m_Size);
	}

}
