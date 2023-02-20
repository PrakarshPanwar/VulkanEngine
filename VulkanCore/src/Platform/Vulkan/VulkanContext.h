#pragma once
#include "VulkanDevice.h"
#include "Platform/Windows/WindowsWindow.h"

namespace VulkanCore {

	namespace VKUtils {
		void SetDebugUtilsObjectName(VkDevice device, VkObjectType objectType, const std::string& debugName, void* object);
	}

	VkResult CreateDebugMarkerEXT(VkDevice device);
	VkResult CreateDebugUtilsEXT(VkInstance instance);

	class VulkanContext
	{
	public:
		VulkanContext(std::shared_ptr<WindowsWindow> window);
		~VulkanContext();

#ifdef VK_RELEASE
		const bool m_EnableValidation = false;
#else
		const bool m_EnableValidation = true;
#endif

		void Init();
		void Destroy();
		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

		static inline VulkanContext* GetCurrentContext() { return s_Instance; }
		static inline VulkanDevice* GetCurrentDevice() { return s_Instance->m_Device.get(); }
		static inline VmaAllocator GetVulkanMemoryAllocator() { return s_Instance->m_VkMemoryAllocator; }
	private:
		void CreateInstance();
		void SetupDebugMessenger();
		void CreateSurface();
		void CreateVulkanMemoryAllocator();

		// Utility Functions
		bool IsDeviceSuitable(VkPhysicalDevice device);
		std::vector<const char*> GetRequiredExtensions();
		bool CheckValidationLayerSupport();
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		void HasGLFWRequiredInstanceExtensions();
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	private:
		VkInstance m_VkInstance;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		VkSurfaceKHR m_VkSurface;

		VmaAllocator m_VkMemoryAllocator;

		const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
		std::vector<const char*> m_DeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_IMAGELESS_FRAMEBUFFER_EXTENSION_NAME,
			VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
			VK_KHR_MAINTENANCE1_EXTENSION_NAME
		};

		std::unique_ptr<VulkanDevice> m_Device; 
		std::shared_ptr<WindowsWindow> m_Window;

		static VulkanContext* s_Instance;

		friend class VulkanDevice;
		friend class ImGuiLayer;
		friend class VulkanSwapChain;
	};

}