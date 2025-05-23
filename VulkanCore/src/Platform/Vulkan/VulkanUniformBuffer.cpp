#include "vulkanpch.h"
#include "VulkanUniformBuffer.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanAllocator.h"

namespace VulkanCore {

	VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size)
		: m_Size(size)
	{
		VulkanAllocator allocator("UniformBuffer");

		VkBufferCreateInfo uniformBufferCreateInfo{};
		uniformBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		uniformBufferCreateInfo.size = m_Size;
		uniformBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		uniformBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		m_MemoryAllocation = allocator.AllocateBuffer(VulkanMemoryType::SharedHeap, uniformBufferCreateInfo, m_VulkanBuffer);

		m_MapDataPtr = allocator.MapMemory<uint8_t>(m_MemoryAllocation);

		m_DescriptorBufferInfo.buffer = m_VulkanBuffer;
		m_DescriptorBufferInfo.range = m_Size;
		m_DescriptorBufferInfo.offset = 0;
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		VulkanAllocator allocator("UniformBuffer");

		allocator.UnmapMemory(m_MemoryAllocation);
		allocator.DestroyBuffer(m_VulkanBuffer, m_MemoryAllocation);
	}

	void VulkanUniformBuffer::WriteData(void* data, uint32_t offset)
	{
		memcpy(m_MapDataPtr, data, m_Size);
	}

}
