#pragma once
#include "VulkanContext.h"

namespace VulkanCore {

	class VulkanIndexBuffer
	{
	public:
		VulkanIndexBuffer() = default;
		VulkanIndexBuffer(void* data, uint32_t size);

		~VulkanIndexBuffer();

		inline VkBuffer GetVulkanBuffer() const { return m_VulkanBuffer; }
	private:
		VkBuffer m_VulkanBuffer = nullptr;
		VmaAllocation m_MemoryAllocation = nullptr;
		uint8_t* m_LocalData = nullptr;
		uint32_t m_Size;
	};

}
