#include "vulkanpch.h"
#include "VulkanVertexBuffer.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"
#include "VulkanAllocator.h"

namespace VulkanCore {

	VulkanVertexBuffer::VulkanVertexBuffer(void* data, uint32_t size)
		: m_Size(size)
	{
		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("VertexBuffer");

		// Staging Buffer
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = m_Size;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAlloc = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, stagingBuffer);

		// Copy/Map Data to Staging Buffer
		uint8_t* dstData = allocator.MapMemory<uint8_t>(stagingBufferAlloc);
		memcpy(dstData, data, size);
		allocator.UnmapMemory(stagingBufferAlloc);

		VkBufferCreateInfo vertexBufferCreateInfo{};
		vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferCreateInfo.size = m_Size;
		vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		m_MemoryAllocation = allocator.AllocateBuffer(vertexBufferCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, m_VulkanBuffer);

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

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("VertexBuffer");
		
		allocator.DestroyBuffer(m_VulkanBuffer, m_MemoryAllocation);
	}

}
