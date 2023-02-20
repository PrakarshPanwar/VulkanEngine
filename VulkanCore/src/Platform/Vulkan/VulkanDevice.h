#pragma once
#include <vma/vk_mem_alloc.h>

#define VIEWPORT_SUPPORT 1

namespace VulkanCore {

	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	void DestroyDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};

	struct QueueFamilyIndices
	{
		uint32_t GraphicsFamily;
		uint32_t ComputeFamily;
		uint32_t PresentFamily;

		bool GraphicsFamilyHasValue = false;
		bool ComputeFamilyHasValue = false;
		bool PresentFamilyHasValue = false;

		bool IsComplete() { return GraphicsFamilyHasValue && ComputeFamilyHasValue && PresentFamilyHasValue; }
	};

	class VulkanDevice
	{
	public:
		VulkanDevice();
		~VulkanDevice();

		inline VkCommandPool GetCommandPool() { return m_CommandPool; }
		inline VkCommandPool GetRenderThreadCommandPool() { return m_RTCommandPool; }
		inline VkDevice GetVulkanDevice() { return m_LogicalDevice; }
		inline VkPhysicalDevice GetPhysicalDevice() { return m_PhysicalDevice; }
		inline VkQueue GetGraphicsQueue() { return m_GraphicsQueue; }
		inline VkQueue GetPresentQueue() { return m_PresentQueue; }
		inline VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() { return m_DeviceProperties; }
		inline VkSampleCountFlagBits GetMSAASampleCount() { return m_MSAASamples; }

		void Init();
		void Destroy();
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		QueueFamilyIndices FindPhysicalQueueFamilies() { return FindQueueFamilies(m_PhysicalDevice); }
		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		bool IsExtensionSupported(const char* extensionName);
		VkCommandBuffer GetCommandBuffer(bool compute = false);
		void FlushCommandBuffer(VkCommandBuffer commandBuffer, bool compute = false);

		void CreateImageWithInfo(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
		VmaAllocation CreateImage(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags properties, VkImage& image);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	private:
		void CreateLogicalDevice();
		void PickPhysicalDevice();
		void CreateCommandPools();
		void SetupDebugMarkers();
	private:
		VkAllocationCallbacks m_AllocationCallbacks;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties m_DeviceProperties;
		VkSampleCountFlagBits m_MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		VkCommandPool m_CommandPool, m_RTCommandPool, m_ComputeCommandPool;

		VkDevice m_LogicalDevice;
		VkQueue m_GraphicsQueue;
		VkQueue m_ComputeQueue;
		VkQueue m_PresentQueue;

		bool m_EnableDebugMarkers = false;
	};

}