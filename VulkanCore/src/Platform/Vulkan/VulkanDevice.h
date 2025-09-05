#pragma once
#include <vk_mem_alloc.h>

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
		uint32_t GraphicsFamily : 8 = _UI8_MAX;
		uint32_t TransferFamily : 8 = _UI8_MAX;
		uint32_t ComputeFamily : 8 = _UI8_MAX;
		uint32_t PresentFamily : 8 = _UI8_MAX;

		const bool HasGraphicsFamily() const { return GraphicsFamily < _UI8_MAX; }
		const bool HasComputeFamily() const { return ComputeFamily < _UI8_MAX; }
		const bool HasTransferFamily() const { return TransferFamily < _UI8_MAX; }
		const bool HasPresentFamily() const { return PresentFamily < _UI8_MAX; }

		const bool IsComplete() const { return HasGraphicsFamily() && HasTransferFamily() && HasComputeFamily() && HasPresentFamily(); }
	};

	enum class VulkanQueueType
	{
		None = 0,
		Graphics,
		Compute,
		Transfer
	};

	struct VulkanCommandBuffer
	{
		VkCommandBuffer CmdBuffer = VK_NULL_HANDLE;
		VulkanQueueType QueueType = VulkanQueueType::None;
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
		const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() const { return m_DeviceProperties; }
		VkSampleCountFlagBits GetMSAASampleCount() const { return m_MSAASamples; }

		void Init();
		void Destroy();
		QueueFamilyIndices FindPhysicalQueueFamilies() { return FindQueueFamilies(m_PhysicalDevice); }
		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

		bool IsExtensionSupported(const char* extensionName) const;
		bool IsInDebugMode() const;
		VulkanCommandBuffer GetCommandBuffer(VulkanQueueType queueType = VulkanQueueType::Graphics) const;
		void FlushCommandBuffer(VulkanCommandBuffer commandBuffer) const;

		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	private:
		VkCommandPool VulkanCommandPool(VulkanQueueType queueType) const;
		VkQueue VulkanQueue(VulkanQueueType queueType) const;

		void CreateLogicalDevice();
		void PickPhysicalDevice();
		void CreateCommandPools();
	private:
		VkAllocationCallbacks m_AllocationCallbacks;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties m_DeviceProperties;
		VkSampleCountFlagBits m_MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		VkCommandPool m_CommandPool, m_RTCommandPool, m_TransferCommandPool, m_ComputeCommandPool;

		VkDevice m_LogicalDevice;
		VkQueue m_GraphicsQueue, m_TransferQueue, m_ComputeQueue, m_PresentQueue;
	};

}
