#pragma once
#include "VulkanContext.h"

namespace VulkanCore {

	class VulkanStorageBuffer
	{
	public:
		VulkanStorageBuffer() = default;
		VulkanStorageBuffer(uint32_t size);

		~VulkanStorageBuffer();
	private:
		VkBuffer m_VulkanBuffer = nullptr;
		VmaAllocation m_MemoryAllocation = nullptr;
		uint32_t m_Size;
	};

}
