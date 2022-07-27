#pragma once
#include "Platform/Windows/WindowsWindow.h"

#define MULTISAMPLING 0

namespace VulkanCore {

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};

	struct QueueFamilyIndices
	{
		uint32_t GraphicsFamily;
		uint32_t PresentFamily;

		bool GraphicsFamilyHasValue = false;
		bool PresentFamilyHasValue = false;

		bool IsComplete() { return GraphicsFamilyHasValue && PresentFamilyHasValue; }
	};

	class VulkanDevice
	{
	public:
#ifdef VK_RELEASE
		const bool m_EnableValidation = false;
#else
		const bool m_EnableValidation = true;
#endif
		VulkanDevice() = default;

		VulkanDevice(WindowsWindow& window);
		~VulkanDevice();

		static VulkanDevice* GetDevice() { return s_Instance; }

		inline VkCommandPool GetCommandPool() { return m_CommandPool; }
		inline VkDevice GetVulkanDevice() { return m_vkDevice; }
		inline VkInstance GetVulkanInstance() { return m_vkInstance; }
		inline VkPhysicalDevice GetPhysicalDevice() { return m_PhysicalDevice; }
		inline VkSurfaceKHR GetSurface() { return m_vkSurface; }
		inline VkQueue GetGraphicsQueue() { return m_vkGraphicsQueue; }
		inline VkQueue GetPresentQueue() { return m_vkPresentQueue; }
		inline VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() { return m_DeviceProperties; }
		inline VkSampleCountFlagBits GetSampleCount() { return m_SampleCount; }

		void Init();
		SwapChainSupportDetails GetSwapChainSupport() { return QuerySwapChainSupport(m_PhysicalDevice); }
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		QueueFamilyIndices FindPhysicalQueueFamilies() { return FindQueueFamilies(m_PhysicalDevice); }
		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		VkCommandBuffer BeginSingleTimeCommands();
		void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

		void CreateImageWithInfo(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	private:
		void CreateInstance();
		void SetupDebugMessenger();
		void SetupAllocationMessenger();
		void CreateSurface();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateCommandPool();

		bool IsDeviceSuitable(VkPhysicalDevice device);
		std::vector<const char*> GetRequiredExtensions();
		bool CheckValidationLayerSupport();
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		void HasGLFWRequiredInstanceExtensions();
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

		VkInstance m_vkInstance;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		VkAllocationCallbacks m_AllocationCallbacks;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties m_DeviceProperties;
		VkSampleCountFlagBits m_SampleCount = VK_SAMPLE_COUNT_1_BIT;
		WindowsWindow& m_Window;
		VkCommandPool m_CommandPool;

		VkDevice m_vkDevice;
		VkSurfaceKHR m_vkSurface;
		VkQueue m_vkGraphicsQueue;
		VkQueue m_vkPresentQueue;

		const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
		const std::vector<const char*> m_DeviceExtensions = { 
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_NV_RAY_TRACING_EXTENSION_NAME };

		static VulkanDevice* s_Instance;

		friend class ImGuiLayer;
	};

}