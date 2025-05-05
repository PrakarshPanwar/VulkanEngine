#include "vulkanpch.h"
#include "VulkanBuffer.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanAllocator.h"
#include "VulkanCore/Renderer/Renderer.h"

namespace VulkanCore {

	VulkanBuffer::VulkanBuffer(uint32_t size, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryPropertyFlags)
		: m_Size(size), m_UsageFlags(usageFlags), m_MemoryUsageFlag(memoryPropertyFlags)
	{
		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("VulkanBuffer");

		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = m_Size;
		bufferCreateInfo.usage = m_UsageFlags;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		m_MemoryAllocation = allocator.AllocateBuffer(VulkanMemoryType::None, bufferCreateInfo, m_VulkanBuffer, m_MemoryUsageFlag);
	}

	VulkanBuffer::~VulkanBuffer()
	{
		if (m_MapDataPtr)
			Unmap();

		vmaDestroyBuffer(VulkanContext::GetVulkanMemoryAllocator(), m_VulkanBuffer, m_MemoryAllocation);
	}

	void VulkanBuffer::Map()
	{
		VulkanAllocator allocator("VulkanBuffer");
		m_MapDataPtr = allocator.MapMemory<uint8_t>(m_MemoryAllocation);
	}

	void VulkanBuffer::Unmap()
	{
		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("VulkanBuffer");

		allocator.UnmapMemory(m_MemoryAllocation);
		m_MapDataPtr = nullptr;
	}

	void VulkanBuffer::WriteToBuffer(void* data, VkDeviceSize size, VkDeviceSize offset)
	{
		VK_CORE_ASSERT(m_MapDataPtr, "Cannot Copy to Unmapped Buffer!");

		if (size == VK_WHOLE_SIZE)
			memcpy(m_MapDataPtr, data, m_Size);

		else
		{
			char* memOffset = (char*)m_MapDataPtr;
			memOffset += offset;
			memcpy(memOffset, data, size);
		}
	}

	VkResult VulkanBuffer::FlushBuffer(VkDeviceSize size, VkDeviceSize offset)
	{
		return vmaFlushAllocation(VulkanContext::GetVulkanMemoryAllocator(), m_MemoryAllocation, offset, size);
	}

	VkDescriptorBufferInfo VulkanBuffer::DescriptorInfo(VkDeviceSize size, VkDeviceSize offset)
	{
		return VkDescriptorBufferInfo{ m_VulkanBuffer, offset, size };
	}

	VkResult VulkanBuffer::Invalidate(VkDeviceSize size, VkDeviceSize offset)
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = m_Memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkInvalidateMappedMemoryRanges(device->GetVulkanDevice(), 1, &mappedRange);
	}

	VkDeviceSize VulkanBuffer::GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment)
	{
		if (minOffsetAlignment > 0)
			return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);

		return instanceSize;
	}

}
