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

		VkCommandPool GetCommandPool() const { return m_CommandPool; }
		VkCommandPool GetRenderThreadCommandPool() const { return m_RTCommandPool; }
		VkDevice GetVulkanDevice() const { return m_LogicalDevice; }
		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		VkQueue GetPresentQueue() const { return m_PresentQueue; }
		VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() { return m_DeviceProperties; }
		VkSampleCountFlagBits GetMSAASampleCount() const { return m_MSAASamples; }

		void Init();
		void Destroy();
		QueueFamilyIndices FindPhysicalQueueFamilies() { return FindQueueFamilies(m_PhysicalDevice); }
		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		bool IsExtensionSupported(const char* extensionName);
		bool IsInDebugMode() const;
		VkCommandBuffer GetCommandBuffer(bool compute = false);
		void FlushCommandBuffer(VkCommandBuffer commandBuffer, bool compute = false);

		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	private:
		void CreateLogicalDevice();
		void PickPhysicalDevice();
		void CreateCommandPools();
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
	};

}
