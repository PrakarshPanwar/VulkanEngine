#pragma once
#include "VulkanContext.h"
#include "VulkanCore/Renderer/UniformBuffer.h"

namespace VulkanCore {

	class VulkanUniformBuffer : public UniformBuffer
	{
	public:
		VulkanUniformBuffer() = default;
		VulkanUniformBuffer(uint32_t size);
		~VulkanUniformBuffer();

		void WriteAndFlushBuffer(void* data, uint32_t offset) override;

		inline const VkDescriptorBufferInfo& GetDescriptorBufferInfo() const { return m_DescriptorBufferInfo; }
	private:
		VkBuffer m_VulkanBuffer = nullptr;
		VmaAllocation m_MemoryAllocation;
		uint8_t* m_dstData = nullptr;
		uint32_t m_Size;
		VkDescriptorBufferInfo m_DescriptorBufferInfo{};
	};

}
