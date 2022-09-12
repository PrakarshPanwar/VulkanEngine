#include "vulkanpch.h"
#include "VulkanDevice.h"

#include "VulkanCore/Core/Log.h"
#include "VulkanCore/Core/Assert.h"

namespace VulkanCore {

	static void VKAPI_CALL InternalAllocationNotification(void* pUserData, size_t size,
		VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
	{
		switch (allocationScope)
		{
		case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND:
			VK_CORE_TRACE("Allocation in Scope Command");
			break;
		case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT:
			VK_CORE_TRACE("Allocation in Scope Object");
			break;
		case VK_SYSTEM_ALLOCATION_SCOPE_CACHE:
			VK_CORE_TRACE("Allocation in Scope Cache");
			break;
		case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE:
			VK_CORE_TRACE("Allocation in Scope Device");
			break;
		case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE:
			VK_CORE_TRACE("Allocation in Scope Instance");
			break;
		default:
			break;
		}
	}

	static void VKAPI_CALL InternalFreeNotification(void* pUserData, size_t size,
		VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
	{
		switch (allocationScope)
		{
		case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND:
			VK_CORE_TRACE("Allocation in Scope Command");
			break;
		case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT:
			VK_CORE_TRACE("Allocation in Scope Object");
			break;
		case VK_SYSTEM_ALLOCATION_SCOPE_CACHE:
			VK_CORE_TRACE("Allocation in Scope Cache");
			break;
		case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE:
			VK_CORE_TRACE("Allocation in Scope Device");
			break;
		case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE:
			VK_CORE_TRACE("Allocation in Scope Instance");
			break;
		default:
			break;
		}
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			VK_CORE_TRACE("Validation Layer: {0}", pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			VK_CORE_INFO("Validation Layer: {0}", pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			VK_CORE_WARN("Validation Layer: {0}", pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			VK_CORE_ERROR("Validation Layer: {0}", pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
			break;
		default:
			break;
		}

		return VK_FALSE;
	}

	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger) 
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);

		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void DestroyDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
			func(instance, debugMessenger, pAllocator);
	}

	VulkanDevice* VulkanDevice::s_Instance;

	void VulkanDevice::SetupAllocationMessenger()
	{
		m_AllocationCallbacks = {};
		m_AllocationCallbacks.pfnInternalAllocation = InternalAllocationNotification;
		m_AllocationCallbacks.pfnInternalFree = InternalFreeNotification;
	}

	VulkanDevice::VulkanDevice(WindowsWindow& window)
		: m_Window(window)
	{
		s_Instance = this;
		Init();
	}

	void VulkanDevice::Init()
	{
		CreateInstance();
		SetupDebugMessenger();
		SetupAllocationMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateCommandPool();
		CreateVMAAllocatorInstance();
	}

	VulkanDevice::~VulkanDevice()
	{
		vkDestroyCommandPool(m_vkDevice, m_CommandPool, nullptr);
		vmaDestroyAllocator(m_VMAAllocator);
		vkDestroyDevice(m_vkDevice, nullptr);

		if (m_EnableValidation)
			DestroyDebugUtilsMessengerEXT(m_vkInstance, m_DebugMessenger, nullptr);

		vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
		vkDestroyInstance(m_vkInstance, nullptr);
	}

	uint32_t VulkanDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
				return i;
		}

		VK_CORE_ASSERT(false, "Failed to find suitable Memory Type!");
	}

	VkFormat VulkanDevice::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
				return format;

			if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
				return format;
		}

		VK_CORE_ASSERT(false, "Failed to find Supported Format!");
	}

	void VulkanDevice::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK_RESULT(vkCreateBuffer(m_vkDevice, &bufferInfo, nullptr, &buffer), "Failed to Create Buffer!");

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_vkDevice, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		VK_CHECK_RESULT(vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &bufferMemory), "Failed to Allocate Buffer Memory!");

		vkBindBufferMemory(m_vkDevice, buffer, bufferMemory, 0);
	}

	VmaAllocation VulkanDevice::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 2 ?
			VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

		allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		allocInfo.preferredFlags = properties;

		VmaAllocation allocation;

		VK_CHECK_RESULT(vmaCreateBuffer(m_VMAAllocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr), "Failed to Create Buffer");
		return allocation;
	}

	VkCommandBuffer VulkanDevice::BeginSingleTimeCommands()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_vkDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		return commandBuffer;
	}

	void VulkanDevice::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(m_vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_vkGraphicsQueue);

		vkFreeCommandBuffers(m_vkDevice, m_CommandPool, 1, &commandBuffer);
	}

	void VulkanDevice::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;  // Optional
		copyRegion.dstOffset = 0;  // Optional
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		EndSingleTimeCommands(commandBuffer);
	}

	void VulkanDevice::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = layerCount;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		EndSingleTimeCommands(commandBuffer);
	}

	void VulkanDevice::CreateImageWithInfo(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
	{
		VK_CHECK_RESULT(vkCreateImage(m_vkDevice, &imageInfo, nullptr, &image), "Failed to Create Image!");

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(m_vkDevice, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		VK_CHECK_RESULT(vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &imageMemory), "Failed to Allocate Image Memory!");
		VK_CHECK_RESULT(vkBindImageMemory(m_vkDevice, image, imageMemory, 0), "Failed to Bind Image Memory!");
	}

	VmaAllocation VulkanDevice::CreateImage(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags properties, VkImage& image)
	{
		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		allocInfo.preferredFlags = properties;

		VmaAllocation imageAllocation;
		VK_CHECK_RESULT(vmaCreateImage(m_VMAAllocator, &imageInfo, &allocInfo, &image, &imageAllocation, nullptr), "Failed to Create Image!");
		return imageAllocation;
	}

	void VulkanDevice::CreateInstance()
	{
		VK_CORE_ASSERT(!m_EnableValidation || CheckValidationLayerSupport(), "Validation Layers requested, but not Available!");

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = m_Window.GetWindowName().c_str();
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 3, 0);
		appInfo.pEngineName = "Vulkan Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 3, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (m_EnableValidation) 
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}

		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_vkInstance), "Failed to Create Vulkan Instance!");

		HasGLFWRequiredInstanceExtensions();
	}

	void VulkanDevice::SetupDebugMessenger()
	{
		if (!m_EnableValidation)
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);
		VK_CHECK_RESULT(CreateDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_DebugMessenger), "Failed to Setup Debug Messenger!");
	}

	void VulkanDevice::CreateSurface()
	{
		VK_CHECK_RESULT(glfwCreateWindowSurface(m_vkInstance, (GLFWwindow*)m_Window.GetNativeWindow(), nullptr, &m_vkSurface),
			"Failed to Create Window Surface!");
	}

	void VulkanDevice::PickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);
		VK_CORE_ASSERT(deviceCount != 0, "Failed to find GPUs with Vulkan Support!");

		VK_CORE_INFO("Device Count: {0}", deviceCount);
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, devices.data());

		for (const auto& device : devices) 
		{
			auto props = VkPhysicalDeviceProperties{};
			vkGetPhysicalDeviceProperties(device, &props);

			if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				m_PhysicalDevice = device;
				break;
			}
		}

		VK_CORE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Failed to find a Suitable GPU!");

		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_DeviceProperties);
		VK_CORE_INFO("Physical Device: {0}", m_DeviceProperties.deviceName);

		auto sampleCount = m_DeviceProperties.limits.framebufferColorSampleCounts & m_DeviceProperties.limits.framebufferDepthSampleCounts;
		m_MSAASamples = VK_SAMPLE_COUNT_8_BIT; // TODO: Get this through some function
	}

	void VulkanDevice::CreateLogicalDevice()
	{
		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.GraphicsFamily, indices.PresentFamily };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.geometryShader = VK_TRUE;
		deviceFeatures.shaderInt64 = VK_TRUE;
		deviceFeatures.multiViewport = VK_TRUE;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

		// Might not really be Necessary anymore because device specific Validation Layers
		// have been deprecated
		if (m_EnableValidation) 
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
		}

		else
			createInfo.enabledLayerCount = 0;

		VK_CHECK_RESULT(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_vkDevice), "Failed to Create Logical Device!");

		vkGetDeviceQueue(m_vkDevice, indices.GraphicsFamily, 0, &m_vkGraphicsQueue);
		vkGetDeviceQueue(m_vkDevice, indices.PresentFamily, 0, &m_vkPresentQueue);
	}

	void VulkanDevice::CreateCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = FindPhysicalQueueFamilies();

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VK_CHECK_RESULT(vkCreateCommandPool(m_vkDevice, &poolInfo, nullptr, &m_CommandPool), "Failed to Create Command Pool!");
	}

	bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices = FindQueueFamilies(device);

		bool extensionsSupported = CheckDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported) 
		{
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return indices.IsComplete() && extensionsSupported && swapChainAdequate 
			&& supportedFeatures.samplerAnisotropy && supportedFeatures.geometryShader;
	}

	std::vector<const char*> VulkanDevice::GetRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (m_EnableValidation)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		return extensions;
	}

	bool VulkanDevice::CheckValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : m_ValidationLayers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
				return false;
		}

		return true;
	}

	QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.GraphicsFamily = i;
				indices.GraphicsFamilyHasValue = true;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_vkSurface, &presentSupport);

			if (queueFamily.queueCount > 0 && presentSupport)
			{
				indices.PresentFamily = i;
				indices.PresentFamilyHasValue = true;
			}

			if (indices.IsComplete())
				break;

			i++;
		}

		return indices;
	}

	void VulkanDevice::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

		createInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		createInfo.pfnUserCallback = DebugCallback;
		createInfo.pUserData = nullptr;
	}

	void VulkanDevice::HasGLFWRequiredInstanceExtensions()
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		VK_CORE_INFO("Available Extensions:");
		std::unordered_set<std::string> available;
		for (const auto& extension : extensions) 
		{
			VK_CORE_TRACE("\t {0}", extension.extensionName);
			available.insert(extension.extensionName);
		}

		VK_CORE_INFO("Required Extensions:");
		auto requiredExtensions = GetRequiredExtensions();
		for (const auto& required : requiredExtensions)
		{
			VK_CORE_WARN("\t {0}", required);
			VK_CORE_ASSERT(available.find(required) != available.end(), "Missing Required GLFW Extension!");
		}
	}

	bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

		for (const auto& extension : availableExtensions)
			requiredExtensions.erase(extension.extensionName);

		return requiredExtensions.empty();
	}

	SwapChainSupportDetails VulkanDevice::QuerySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_vkSurface, &details.Capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vkSurface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			details.Formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vkSurface, &formatCount, details.Formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vkSurface, &presentModeCount, nullptr);

		if (presentModeCount != 0) 
		{
			details.PresentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vkSurface, &presentModeCount, details.PresentModes.data());
		}

		return details;
	}

	void VulkanDevice::CreateVMAAllocatorInstance()
	{
		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

		VmaAllocatorCreateInfo allocatorInfo;
		allocatorInfo.device = m_vkDevice;
		allocatorInfo.instance = m_vkInstance;
		allocatorInfo.physicalDevice = m_PhysicalDevice;
		allocatorInfo.pVulkanFunctions = &vulkanFunctions;
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
		allocatorInfo.pAllocationCallbacks = nullptr;
		allocatorInfo.pDeviceMemoryCallbacks = nullptr;
		allocatorInfo.pHeapSizeLimit = nullptr;
		allocatorInfo.pTypeExternalMemoryHandleTypes = nullptr;

		vmaCreateAllocator(&allocatorInfo, &m_VMAAllocator);
	}

}