#include "vulkanpch.h"
#include "VulkanAllocator.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

namespace VulkanCore {

	namespace Utils {

		VkMemoryPropertyFlags VulkanMemoryFlags(VmaMemoryUsage usage)
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
		allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		allocInfo.preferredFlags = Utils::VulkanMemoryFlags(usage);

		VmaAllocation vmaAllocation;
		VK_CHECK_RESULT(vmaCreateBuffer(m_VkMemoryAllocator, &bufInfo, &allocInfo, &buffer, &vmaAllocation, nullptr), "{0}: Failed to Allocate Buffer!", m_DebugName);
		return vmaAllocation;
	}

	VmaAllocation VulkanAllocator::AllocateImage(const VkImageCreateInfo& imgInfo, VmaMemoryUsage usage, VkImage& image)
	{
		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = usage;
		allocInfo.preferredFlags = Utils::VulkanMemoryFlags(usage);

		VmaAllocation vmaAllocation;
		VK_CHECK_RESULT(vmaCreateImage(m_VkMemoryAllocator, &imgInfo, &allocInfo, &image, &vmaAllocation, nullptr), "{0}: Failed to Allocate Image!", m_DebugName);
		return vmaAllocation;
	}

	template<typename T>
	T* VulkanAllocator::MapMemory(VmaAllocation allocation)
	{
		void* mappedData;
		VK_CHECK_RESULT(vmaMapMemory(m_VkMemoryAllocator, allocation, &mappedData), "{}: Failed to Map Memory!", m_DebugName);

		static_assert(std::integral<T>, "Type is not Integral");
		return (T*)mappedData;
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