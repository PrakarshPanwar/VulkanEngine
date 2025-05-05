#include "vulkanpch.h"
#include "VulkanIndexBuffer.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "VulkanAllocator.h"

namespace VulkanCore {

	VulkanIndexBuffer::VulkanIndexBuffer(void* data, uint32_t size)
		: m_Size(size)
	{
		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("IndexBuffer");

		// Staging Buffer
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = m_Size;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAlloc = allocator.AllocateBuffer(VulkanMemoryType::HostLocal, bufferCreateInfo, stagingBuffer, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);

		// Copy/Map Data to Staging Buffer
		uint8_t* dstData = allocator.MapMemory<uint8_t>(stagingBufferAlloc);
		memcpy(dstData, data, size);
		allocator.UnmapMemory(stagingBufferAlloc);

		VkBufferCreateInfo indexBufferCreateInfo{};
		indexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		indexBufferCreateInfo.size = m_Size;
		indexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		m_MemoryAllocation = allocator.AllocateBuffer(VulkanMemoryType::DeviceLocal, indexBufferCreateInfo, m_VulkanBuffer, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

		VkCommandBuffer copyCmd = device->GetCommandBuffer();

		VkBufferCopy copyRegion{};
		copyRegion.size = m_Size;

		vkCmdCopyBuffer(copyCmd,
			stagingBuffer,
			m_VulkanBuffer,
			1,
			&copyRegion
		);

		device->FlushCommandBuffer(copyCmd);

		allocator.DestroyBuffer(stagingBuffer, stagingBufferAlloc);
	}

	VulkanIndexBuffer::VulkanIndexBuffer(uint32_t size)
		: m_Size(size)
	{
		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("IndexBuffer");

		// Index Buffer
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = m_Size;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		m_MemoryAllocation = allocator.AllocateBuffer(VulkanMemoryType::HostCached, bufferCreateInfo, m_VulkanBuffer);

		// Map Data to Index Buffer
		m_MapDataPtr = allocator.MapMemory<uint8_t>(m_MemoryAllocation);
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("IndexBuffer");

		if (m_MapDataPtr)
			allocator.UnmapMemory(m_MemoryAllocation);

		allocator.DestroyBuffer(m_VulkanBuffer, m_MemoryAllocation);
	}

	void VulkanIndexBuffer::WriteData(void* data, uint32_t offset)
	{
		if (data)
			memcpy(m_MapDataPtr, data, m_Size);
	}

}
