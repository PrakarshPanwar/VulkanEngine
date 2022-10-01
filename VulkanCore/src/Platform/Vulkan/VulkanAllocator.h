#pragma once
#include "VulkanDevice.h"

namespace VulkanCore {

	class VulkanAllocator
	{
	public:
		VulkanAllocator(const char* debugName);
		~VulkanAllocator();

		VmaAllocation AllocateBuffer(const VkBufferCreateInfo& bufInfo, VmaMemoryUsage usage, VkBuffer& buffer);
		VmaAllocation AllocateImage(const VkImageCreateInfo& imgInfo, VmaMemoryUsage usage, VkImage& image);

		template<typename T>
		[[nodiscard]] T* MapMemory(VmaAllocation allocation);

		void UnmapMemory(VmaAllocation allocation);

		void DestroyBuffer(VkBuffer& buffer, VmaAllocation allocation);
		void DestroyImage(VkImage& image, VmaAllocation allocation);
	private:
		const VmaAllocator m_vkAllocator = VulkanDevice::GetDevice()->GetVulkanAllocator();
		std::string_view m_DebugName;
	};

}