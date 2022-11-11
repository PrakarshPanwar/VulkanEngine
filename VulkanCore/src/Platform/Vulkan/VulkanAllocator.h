#pragma once
#include "VulkanContext.h"

namespace VulkanCore {

	class VulkanAllocator
	{
	public:
		VulkanAllocator(const char* debugName);
		~VulkanAllocator();

		VmaAllocation AllocateBuffer(const VkBufferCreateInfo& bufInfo, VmaMemoryUsage usage, VkBuffer& buffer);
		VmaAllocation AllocateImage(const VkImageCreateInfo& imgInfo, VmaMemoryUsage usage, VkImage& image);

		template<typename T>
		T* MapMemory(VmaAllocation allocation)
		{
			void* mappedData;
			VK_CHECK_RESULT(vmaMapMemory(m_VkMemoryAllocator, allocation, &mappedData), "{}: Failed to Map Memory!", m_DebugName);

			static_assert(std::integral<T>, "Type is not Integral");
			return (T*)mappedData;
		}

		void UnmapMemory(VmaAllocation allocation);

		void DestroyBuffer(VkBuffer& buffer, VmaAllocation allocation);
		void DestroyImage(VkImage& image, VmaAllocation allocation);
	private:
		const VmaAllocator m_VkMemoryAllocator = VulkanContext::GetVulkanMemoryAllocator();
		std::string_view m_DebugName;
	};

}