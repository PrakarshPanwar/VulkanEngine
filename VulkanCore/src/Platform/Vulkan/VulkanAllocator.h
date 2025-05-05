#pragma once
#include "VulkanContext.h"

namespace VulkanCore {

	enum class VulkanMemoryType
	{
		None = 0,
		DeviceLocal, // GPU VRAM, fastest for GPU access
		HostLocal, // CPU RAM
		HostCached, // CPU Cached for faster reads
		SharedHeap // Shared memory between CPU and GPU(Ideal use Uniform, Storage Buffers)
	};

	struct AllocationStats
	{
		uint32_t AllocatedBytes = 0;
		uint32_t AllocationCount = 0;
	};

	class VulkanAllocator
	{
	public:
		VulkanAllocator(const char* debugName);
		~VulkanAllocator();

		VmaAllocation AllocateBuffer(VulkanMemoryType memoryType, const VkBufferCreateInfo& bufInfo, VkBuffer& buffer, VmaMemoryUsage usage = VMA_MEMORY_USAGE_AUTO);
		VmaAllocation AllocateImage(const VkImageCreateInfo& imgInfo, VmaMemoryUsage usage, VkImage& image);

		template<typename T>
		T* MapMemory(VmaAllocation allocation) requires std::integral<T>
		{
			void* mappedData;
			VK_CHECK_RESULT(vmaMapMemory(m_VkMemoryAllocator, allocation, &mappedData), "{}: Failed to Map Memory!", m_DebugName);

			return (T*)mappedData;
		}

		void UnmapMemory(VmaAllocation allocation);
		void DestroyBuffer(VkBuffer& buffer, VmaAllocation allocation);
		void DestroyImage(VkImage& image, VmaAllocation allocation);

		void WriteAllocatorStats() const;
		static inline const AllocationStats& GetAllocationStats() { return s_Data; }
	private:
		const VmaAllocator m_VkMemoryAllocator = VulkanContext::GetVulkanMemoryAllocator();
		std::string_view m_DebugName;

		static AllocationStats s_Data;
	};

}
