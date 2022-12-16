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
	}

	VulkanStorageBuffer::~VulkanStorageBuffer()
	{
		VulkanAllocator allocator("StorageBuffer");
		allocator.DestroyBuffer(m_VulkanBuffer, m_MemoryAllocation);
	}

}
