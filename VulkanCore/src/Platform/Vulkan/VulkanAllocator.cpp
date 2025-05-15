#include "vulkanpch.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanAllocator.h"

namespace VulkanCore {

	namespace Utils {

		static VkMemoryPropertyFlags VulkanMemoryFlags(VmaMemoryUsage usage)
		{
			switch (usage)
			{
			case VMA_MEMORY_USAGE_AUTO:               return 0;
			case VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE: return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			case VMA_MEMORY_USAGE_AUTO_PREFER_HOST:   return VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			default:
				VK_CORE_ASSERT(false, "Could not find necessary Format!");
				return 0;
			}
		}

		static VkMemoryPropertyFlags VulkanMemoryFlags(VulkanMemoryType memoryType)
		{
			switch (memoryType)
			{
			case VulkanMemoryType::None:		return 0;
			case VulkanMemoryType::DeviceLocal: return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			case VulkanMemoryType::HostLocal:	return VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			case VulkanMemoryType::HostCached:  return VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			case VulkanMemoryType::SharedHeap:  return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			default:
				VK_CORE_ASSERT(false, "Could not find necessary Format!");
				return 0;
			}
		}

		static VmaAllocationCreateFlags VulkanAllocationFlags(VulkanMemoryType memoryType)
		{
			switch (memoryType)
			{
			case VulkanMemoryType::None:        return 0;
			case VulkanMemoryType::DeviceLocal: return 0;
			case VulkanMemoryType::HostLocal:   return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			case VulkanMemoryType::HostCached:  return VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			case VulkanMemoryType::SharedHeap:  return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			default:
				VK_CORE_ASSERT(false, "Could not find necessary Format!");
				return 0;
			}
		}

	}

	AllocationStats VulkanAllocator::s_Data;

	VulkanAllocator::VulkanAllocator(const char* debugName)
		: m_DebugName(debugName)
	{
	}

	VulkanAllocator::~VulkanAllocator()
	{
	}

	VmaAllocation VulkanAllocator::AllocateBuffer(VulkanMemoryType memoryType, const VkBufferCreateInfo& bufInfo, VkBuffer& buffer, VmaMemoryUsage usage)
	{
		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = usage;
		allocInfo.flags = Utils::VulkanAllocationFlags(memoryType);
		allocInfo.preferredFlags = Utils::VulkanMemoryFlags(memoryType);

		VmaAllocation vmaAllocation;
		VmaAllocationInfo vmaAllocInfo = {};
		VK_CHECK_RESULT(vmaCreateBuffer(m_VkMemoryAllocator, &bufInfo, &allocInfo, &buffer, &vmaAllocation, &vmaAllocInfo), "{0}: Failed to Allocate Buffer!", m_DebugName);

		VkMemoryPropertyFlags memoryPropertyFlags{};
		vmaGetMemoryTypeProperties(m_VkMemoryAllocator, vmaAllocInfo.memoryType, &memoryPropertyFlags);
		VK_CORE_ASSERT(memoryPropertyFlags > 0, "Buffer Memory Type is Invalid!");

		// Update Stats
		s_Data.AllocatedBytes += vmaAllocInfo.size;
		s_Data.AllocationCount += 1;

		VK_CORE_DEBUG("Buffer Size({0}): {1}", m_DebugName, vmaAllocInfo.size);
		
		return vmaAllocation;
	}

	VmaAllocation VulkanAllocator::AllocateImage(const VkImageCreateInfo& imgInfo, VmaMemoryUsage usage, VkImage& image)
	{
		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = usage;
		allocInfo.preferredFlags = Utils::VulkanMemoryFlags(usage);

		VmaAllocation vmaAllocation;
		VmaAllocationInfo vmaAllocInfo = {};
		VK_CHECK_RESULT(vmaCreateImage(m_VkMemoryAllocator, &imgInfo, &allocInfo, &image, &vmaAllocation, &vmaAllocInfo), "{0}: Failed to Allocate Image!", m_DebugName);

		VkMemoryPropertyFlags memoryPropertyFlags{};
		vmaGetMemoryTypeProperties(m_VkMemoryAllocator, vmaAllocInfo.memoryType, &memoryPropertyFlags);
		VK_CORE_ASSERT(memoryPropertyFlags == 1, "Image Memory Type is not Device Local!");

		// Update Stats
		s_Data.AllocatedBytes += vmaAllocInfo.size;
		s_Data.AllocationCount += 1;

		VK_CORE_DEBUG("Image Size({0}): {1}", m_DebugName, vmaAllocInfo.size);
		
		return vmaAllocation;
	}

	void VulkanAllocator::UnmapMemory(VmaAllocation allocation)
	{
		vmaUnmapMemory(m_VkMemoryAllocator, allocation);
	}

	void VulkanAllocator::DestroyBuffer(VkBuffer& buffer, VmaAllocation allocation)
	{
		vmaDestroyBuffer(m_VkMemoryAllocator, buffer, allocation);
	}

	void VulkanAllocator::DestroyImage(VkImage& image, VmaAllocation allocation)
	{
		vmaDestroyImage(m_VkMemoryAllocator, image, allocation);
	}

	void VulkanAllocator::WriteAllocatorStats() const
	{
		char* statsString = nullptr;
		vmaBuildStatsString(m_VkMemoryAllocator, &statsString, VK_TRUE);

		std::ofstream statsFile("../../VulkanProfiler/Scripts/VulkanAllocatorLog.json", std::ios::out | std::ios::binary);
		if (statsFile.is_open())
		{
			statsFile.write(statsString, strlen(statsString));
			statsFile.flush();
			statsFile.close();
		}
	}

}
