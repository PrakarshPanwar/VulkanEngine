#include "vulkanpch.h"
#include "VulkanVertexBuffer.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Renderer.h"
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

		VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; // Acceleration Structure

		VkBufferCreateInfo vertexBufferCreateInfo{};
		vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferCreateInfo.size = m_Size;
		vertexBufferCreateInfo.usage = usage;
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

	VulkanVertexBuffer::VulkanVertexBuffer(uint32_t size)
		: m_Size(size)
	{
		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("VertexBuffer");

		// Vertex Buffer
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = m_Size;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		m_MemoryAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, m_VulkanBuffer);

		// Map Data to Vertex Buffer
		m_MapDataPtr = allocator.MapMemory<uint8_t>(m_MemoryAllocation);
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		Renderer::SubmitResourceFree([mapPtr = m_MapDataPtr, memoryAlloc = m_MemoryAllocation, vulkanBuffer = m_VulkanBuffer]() mutable
		{
			auto device = VulkanContext::GetCurrentDevice();
			VulkanAllocator allocator("VertexBuffer");
		
			if (mapPtr)
				allocator.UnmapMemory(memoryAlloc);

			allocator.DestroyBuffer(vulkanBuffer, memoryAlloc);
		});
	}

	void VulkanVertexBuffer::WriteData(void* data, uint32_t offset)
	{
		if (data)
		{
			memcpy(m_MapDataPtr, data, m_Size);
			vmaFlushAllocation(VulkanContext::GetVulkanMemoryAllocator(), m_MemoryAllocation, (VkDeviceSize)offset, (VkDeviceSize)m_Size);
		}
	}

	uint64_t VulkanVertexBuffer::GetVulkanBufferDeviceAddress(uint64_t offset) const
	{
		auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
		bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAddressInfo.buffer = m_VulkanBuffer;

		VkDeviceOrHostAddressConstKHR deviceConstAddress{};
		deviceConstAddress.deviceAddress = vkGetBufferDeviceAddressKHR(vulkanDevice, &bufferDeviceAddressInfo) + offset;
		return deviceConstAddress.deviceAddress;
	}

}
