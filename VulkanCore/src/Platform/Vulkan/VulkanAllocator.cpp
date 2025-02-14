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

		static VmaAllocationCreateFlags VulkanAllocationFlags(VmaMemoryUsage usage)
		{
			switch (usage)
			{
			case VMA_MEMORY_USAGE_AUTO:               return 0;
			case VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE: return 0;
			case VMA_MEMORY_USAGE_AUTO_PREFER_HOST:   return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			default:
				VK_CORE_ASSERT(false, "Could not find necessary Format!");
				return 0;
			}
		}

	}

	VulkanAllocator::VulkanAllocator(const char* debugName)
		: m_DebugName(debugName)
	{
	}

	VulkanAllocator::~VulkanAllocator()
	{
	}

	VmaAllocation VulkanAllocator::AllocateBuffer(const VkBufferCreateInfo& bufInfo, VmaMemoryUsage usage, VkBuffer& buffer)
	{
		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = usage;
		allocInfo.flags = Utils::VulkanAllocationFlags(usage);
		allocInfo.preferredFlags = Utils::VulkanMemoryFlags(usage);

		VmaAllocation vmaAllocation;
		VK_CHECK_RESULT(vmaCreateBuffer(m_VkMemoryAllocator, &bufInfo, &allocInfo, &buffer, &vmaAllocation, nullptr), "{0}: Failed to Allocate Buffer!", m_DebugName);
		
		VmaAllocationInfo vmaAllocInfo = {};
		vmaGetAllocationInfo(m_VkMemoryAllocator, vmaAllocation, &vmaAllocInfo);

		VK_CORE_TRACE("Buffer Size({0}): {1}", m_DebugName, vmaAllocInfo.size);
		
		return vmaAllocation;
	}

	VmaAllocation VulkanAllocator::AllocateImage(const VkImageCreateInfo& imgInfo, VmaMemoryUsage usage, VkImage& image)
	{
		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = usage;
		allocInfo.preferredFlags = Utils::VulkanMemoryFlags(usage);

		VmaAllocation vmaAllocation;
		VK_CHECK_RESULT(vmaCreateImage(m_VkMemoryAllocator, &imgInfo, &allocInfo, &image, &vmaAllocation, nullptr), "{0}: Failed to Allocate Image!", m_DebugName);
		
		VmaAllocationInfo vmaAllocInfo = {};
		vmaGetAllocationInfo(m_VkMemoryAllocator, vmaAllocation, &vmaAllocInfo);

		VK_CORE_TRACE("Image Size({0}): {1}", m_DebugName, vmaAllocInfo.size);
		
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

}