#pragma once
#include "VulkanContext.h"
#include "VulkanCore/Renderer/IndexBuffer.h"

namespace VulkanCore {

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer() = default;
		VulkanIndexBuffer(void* data, uint32_t size);
		VulkanIndexBuffer(uint32_t size);
		~VulkanIndexBuffer();

		void WriteData(void* data, uint32_t offset) override;
		VkBuffer GetVulkanBuffer() const { return m_VulkanBuffer; }
		uint8_t* GetMapPointer() const { return m_MapDataPtr; }
	private:
		VkBuffer m_VulkanBuffer = nullptr;
		VmaAllocation m_MemoryAllocation = nullptr;
		uint8_t* m_MapDataPtr = nullptr;
		uint32_t m_Size;
	};

}
