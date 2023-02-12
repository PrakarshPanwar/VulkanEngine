#pragma once
#include "VulkanContext.h"

namespace VulkanCore {

	class VulkanVertexBuffer
	{
	public:
		VulkanVertexBuffer() = default;
		VulkanVertexBuffer(void* data, uint32_t size);
		VulkanVertexBuffer(uint32_t size);

		~VulkanVertexBuffer();

		void WriteData(void* data, VkDeviceSize offset);
		inline VkBuffer GetVulkanBuffer() const { return m_VulkanBuffer; }
	private:
		VkBuffer m_VulkanBuffer = nullptr;
		VmaAllocation m_MemoryAllocation = nullptr;
		uint8_t* m_LocalData = nullptr;
		uint8_t* m_MappedPtr = nullptr;
		uint32_t m_Size;
	};

}
