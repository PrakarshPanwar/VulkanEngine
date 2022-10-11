#include "vulkanpch.h"
#include "VulkanBuffer.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"
#include "VulkanAllocator.h"

namespace VulkanCore {

	VulkanBuffer::VulkanBuffer(VkDeviceSize instanceSize, uint32_t instanceCount, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize minOffsetAlignment)
		: m_InstanceSize(instanceSize), m_InstanceCount(instanceCount), m_UsageFlags(usageFlags),
		m_MemoryPropertyFlags(memoryPropertyFlags)
	{
		auto device = VulkanDevice::GetDevice();
		VulkanAllocator allocator("Buffer");

		m_AlignmentSize = GetAlignment(m_InstanceSize, minOffsetAlignment);
		m_BufferSize = m_AlignmentSize * m_InstanceCount;

		// TODO: To use VulkanAllocator
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = m_BufferSize;
		bufferInfo.usage = m_UsageFlags;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		m_MemoryAllocation = device->CreateBuffer(m_BufferSize, m_UsageFlags, m_MemoryPropertyFlags, m_Buffer);
	}

	VulkanBuffer::~VulkanBuffer()
	{
		auto device = VulkanDevice::GetDevice();

		Unmap();
		vmaDestroyBuffer(device->GetVulkanAllocator(), m_Buffer, m_MemoryAllocation);
	}

	VkResult VulkanBuffer::MapOld(VkDeviceSize size, VkDeviceSize offset)
	{
		auto device = VulkanDevice::GetDevice();

		VK_CORE_ASSERT(m_Buffer && m_Memory, "Called Map on Buffer before its creation!");
		return vkMapMemory(device->GetVulkanDevice(), m_Memory, offset, size, 0, &m_dstMapped);
	}

	VkResult VulkanBuffer::Map()
	{
		auto device = VulkanDevice::GetDevice();

		VK_CORE_ASSERT(m_Buffer, "Called Map on Buffer before its creation!");
		return vmaMapMemory(device->GetVulkanAllocator(), m_MemoryAllocation, &m_dstMapped);
	}

	void VulkanBuffer::UnmapOld()
	{
		auto device = VulkanDevice::GetDevice();

		if (m_dstMapped)
		{
			vkUnmapMemory(device->GetVulkanDevice(), m_Memory);
			m_dstMapped = nullptr;
		}
	}

	void VulkanBuffer::Unmap()
	{
		auto device = VulkanDevice::GetDevice();

		if (m_dstMapped)
		{
			vmaUnmapMemory(device->GetVulkanAllocator(), m_MemoryAllocation);
			m_dstMapped = nullptr;
		}
	}

	void VulkanBuffer::WriteToBuffer(void* data, VkDeviceSize size, VkDeviceSize offset)
	{
		VK_CORE_ASSERT(m_dstMapped, "Cannot Copy to Unmapped Buffer!");

		if (size == VK_WHOLE_SIZE)
			memcpy(m_dstMapped, data, m_BufferSize);

		else
		{
			char* memOffset = (char*)m_dstMapped;
			memOffset += offset;
			memcpy(memOffset, data, size);
		}
	}

	VkResult VulkanBuffer::FlushBufferOld(VkDeviceSize size, VkDeviceSize offset)
	{
		auto device = VulkanDevice::GetDevice();

		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = m_Memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkFlushMappedMemoryRanges(device->GetVulkanDevice(), 1, &mappedRange);
	}

	VkResult VulkanBuffer::FlushBuffer(VkDeviceSize size, VkDeviceSize offset)
	{
		auto device = VulkanDevice::GetDevice();
		return vmaFlushAllocation(device->GetVulkanAllocator(), m_MemoryAllocation, offset, size);
	}

	VkDescriptorBufferInfo VulkanBuffer::DescriptorInfo(VkDeviceSize size, VkDeviceSize offset)
	{
		return VkDescriptorBufferInfo{ m_Buffer, offset, size };
	}

	VkResult VulkanBuffer::Invalidate(VkDeviceSize size, VkDeviceSize offset)
	{
		auto device = VulkanDevice::GetDevice();

		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = m_Memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkInvalidateMappedMemoryRanges(device->GetVulkanDevice(), 1, &mappedRange);
	}

	void VulkanBuffer::WriteToIndex(void* data, int index)
	{
		WriteToBuffer(data, m_InstanceSize, index * m_AlignmentSize);
	}

	VkResult VulkanBuffer::FlushIndex(int index)
	{
		return FlushBufferOld(m_AlignmentSize, index * m_AlignmentSize);
	}

	VkDescriptorBufferInfo VulkanBuffer::DescriptorInfoForIndex(int index)
	{
		return DescriptorInfo(m_AlignmentSize, index * m_AlignmentSize);
	}

	VkResult VulkanBuffer::InvalidateIndex(int index)
	{
		return Invalidate(m_AlignmentSize, index * m_AlignmentSize);
	}

	VkDeviceSize VulkanBuffer::GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment)
	{
		if (minOffsetAlignment > 0)
			return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);

		return instanceSize;
	}

}