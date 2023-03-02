#include "vulkanpch.h"
#include "VulkanContext.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Application.h"

namespace VulkanCore {

	// Debug Marker Function Pointers
	PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXT = VK_NULL_HANDLE;
	PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = VK_NULL_HANDLE;

	VkResult CreateDebugMarkerEXT(VkDevice device)
	{
		vkDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");

		if (vkDebugMarkerSetObjectNameEXT == nullptr)
			return VK_ERROR_EXTENSION_NOT_PRESENT;

		return VK_SUCCESS;
	}

	VkResult CreateDebugUtilsEXT(VkInstance instance)
	{
		vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");

		if (vkSetDebugUtilsObjectNameEXT == nullptr)
			return VK_ERROR_EXTENSION_NOT_PRESENT;

		return VK_SUCCESS;
	}

	namespace VKUtils {

		void SetDebugUtilsObjectName(VkDevice device, VkObjectType objectType, const std::string& debugName, void* object)
		{
			if (vkSetDebugUtilsObjectNameEXT == nullptr)
				return;

			VkDebugUtilsObjectNameInfoEXT debugUtilsNameInfo{};
			debugUtilsNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			debugUtilsNameInfo.objectType = objectType;
			debugUtilsNameInfo.objectHandle = (uint64_t)object;
			debugUtilsNameInfo.pObjectName = debugName.c_str();

			vkSetDebugUtilsObjectNameEXT(device, &debugUtilsNameInfo);

			if (vkDebugMarkerSetObjectNameEXT == nullptr)
				return;

			VkDebugMarkerObjectNameInfoEXT debugMarkerNameInfo{};
			debugMarkerNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
			debugMarkerNameInfo.objectType = (VkDebugReportObjectTypeEXT)objectType;
			debugMarkerNameInfo.object = (uint64_t)object;
			debugMarkerNameInfo.pObjectName = debugName.c_str();
				
			vkDebugMarkerSetObjectNameEXT(device, &debugMarkerNameInfo);
		}

	}

	namespace Utils {

		std::string VkObjectTypeToString(VkObjectType objectType)
		{
			switch (objectType)
			{
			case VK_OBJECT_TYPE_UNKNOWN:	     return "Unknown";
			case VK_OBJECT_TYPE_BUFFER:		     return "Buffer";
			case VK_OBJECT_TYPE_COMMAND_POOL:    return "Command Pool";
			case VK_OBJECT_TYPE_COMMAND_BUFFER:  return "Command Buffer";
			case VK_OBJECT_TYPE_DEVICE:		     return "Device";
			case VK_OBJECT_TYPE_DESCRIPTOR_POOL: return "Descriptor Pool";
			case VK_OBJECT_TYPE_IMAGE:		     return "Image";
			case VK_OBJECT_TYPE_IMAGE_VIEW:		 return "Image View";
			case VK_OBJECT_TYPE_PIPELINE:		 return "Pipeline";
			case VK_OBJECT_TYPE_SAMPLER:		 return "Image Sampler";
			case VK_OBJECT_TYPE_FRAMEBUFFER:     return "Framebuffer";
			case VK_OBJECT_TYPE_RENDER_PASS:	 return "Render Pass";
			case VK_OBJECT_TYPE_SHADER_MODULE:   return "Shader Module";
			case VK_OBJECT_TYPE_QUEUE:			 return "Queue";
			default:
				VK_CORE_ASSERT(false, "Object Type not present in this scope");
				return "Unsupported";
			}
		}

		std::string VkDebugUtilsMessageSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity)
		{
			switch (messageSeverity)
			{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "Verbose";
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:	  return "Info";
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "Warning";
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:	  return "Error";
			default:											  return "Unknown";
			}
		}

	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugUtilsCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		std::string labels, objects;
		if (pCallbackData->cmdBufLabelCount)
		{
			labels = fmt::format("\tLabels({}): \n", pCallbackData->cmdBufLabelCount);
			for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; ++i)
			{
				const auto& label = pCallbackData->pCmdBufLabels[i];
				const std::string colorStr = fmt::format("[ {0}, {1}, {2}, {3} ]", label.color[0], label.color[1], label.color[2], label.color[3]);
				labels.append(fmt::format("\t\t- Command Buffer Label[{0}]: name: {1}, color: {2}\n", i, label.pLabelName ? label.pLabelName : "NULL", colorStr));
			}
		}

		if (pCallbackData->objectCount)
		{
			objects = fmt::format("\tObjects({}): \n", pCallbackData->objectCount);
			for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
			{
				const auto& object = pCallbackData->pObjects[i];
				objects.append(fmt::format("\t\t- Object[{0}]: name: {1}, type: {2}, handle: {3:#x}\n", i, object.pObjectName ? object.pObjectName : "NULL", Utils::VkObjectTypeToString(object.objectType), object.objectHandle));
			}
		}

		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			VK_CORE_WARN("Validation Layer: {0} message: \n\t{1}\n {2} {3}", Utils::VkDebugUtilsMessageSeverity(messageSeverity), pCallbackData->pMessage, labels, objects);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			VK_CORE_ERROR("Validation Layer: {0} message: \n\t{1}\n {2} {3}", Utils::VkDebugUtilsMessageSeverity(messageSeverity), pCallbackData->pMessage, labels, objects);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
			break;
		default:
			break;
		}

		return VK_FALSE;
	}

	VulkanContext* VulkanContext::s_Instance = nullptr;

	VulkanContext::VulkanContext(std::shared_ptr<WindowsWindow> window)
		: m_Window(window)
	{
		s_Instance = this;

		Init();
		m_Device = std::make_unique<VulkanDevice>();
		CreateVulkanMemoryAllocator();
	}

	VulkanContext::~VulkanContext()
	{
		Destroy();
	}

	void VulkanContext::Init()
	{
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
	}

	void VulkanContext::CreateInstance()
	{
		VK_CORE_ASSERT(!m_EnableValidation || CheckValidationLayerSupport(), "Validation Layers requested, but not Available!");

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = m_Window->GetWindowName().c_str();
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

		VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_VkInstance), "Failed to Create Vulkan Instance!");

		HasGLFWRequiredInstanceExtensions();
	}

	void VulkanContext::SetupDebugMessenger()
	{
		if (!m_EnableValidation)
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);
		VK_CHECK_RESULT(CreateDebugUtilsMessengerEXT(m_VkInstance, &createInfo, nullptr, &m_DebugMessenger), "Failed to Setup Debug Messenger!");
		VK_CHECK_RESULT(CreateDebugUtilsEXT(m_VkInstance), "Failed to Set Debug Utils!");
	}

	void VulkanContext::CreateSurface()
	{
		VK_CHECK_RESULT(glfwCreateWindowSurface(m_VkInstance, (GLFWwindow*)m_Window->GetNativeWindow(), nullptr, &m_VkSurface),
			"Failed to Create Window Surface!");
	}

	void VulkanContext::CreateVulkanMemoryAllocator()
	{
		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

		auto physicalDevice = m_Device->GetPhysicalDevice();

		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		VmaAllocatorCreateInfo allocatorInfo;
		allocatorInfo.device = m_Device->GetVulkanDevice();
		allocatorInfo.instance = m_VkInstance;
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.pVulkanFunctions = &vulkanFunctions;
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
		allocatorInfo.pAllocationCallbacks = nullptr;
		allocatorInfo.pDeviceMemoryCallbacks = nullptr;
		allocatorInfo.pHeapSizeLimit = nullptr;
		allocatorInfo.pTypeExternalMemoryHandleTypes = nullptr;

		vmaCreateAllocator(&allocatorInfo, &m_VkMemoryAllocator);
	}

	bool VulkanContext::IsDeviceSuitable(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices = m_Device->FindQueueFamilies(device);

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

	std::vector<const char*> VulkanContext::GetRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (m_EnableValidation)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		return extensions;
	}

	bool VulkanContext::CheckValidationLayerSupport()
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

	void VulkanContext::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
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

		createInfo.pfnUserCallback = VulkanDebugUtilsCallback;
		createInfo.pUserData = nullptr;
	}

	void VulkanContext::HasGLFWRequiredInstanceExtensions()
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

	bool VulkanContext::CheckDeviceExtensionSupport(VkPhysicalDevice device)
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

	SwapChainSupportDetails VulkanContext::QuerySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_VkSurface, &details.Capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VkSurface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			details.Formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VkSurface, &formatCount, details.Formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_VkSurface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			details.PresentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_VkSurface, &presentModeCount, details.PresentModes.data());
		}

		return details;
	}

	void VulkanContext::Destroy()
	{
		vmaDestroyAllocator(m_VkMemoryAllocator);
		m_Device->Destroy();

		if (m_EnableValidation)
			DestroyDebugUtilsMessengerEXT(m_VkInstance, m_DebugMessenger, nullptr);

		vkDestroySurfaceKHR(m_VkInstance, m_VkSurface, nullptr);
		vkDestroyInstance(m_VkInstance, nullptr);
	}

}