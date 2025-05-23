#include "vulkanpch.h"
#include "VulkanContext.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Application.h"

namespace VulkanCore {

	// Debug Utils Marker Function Pointers
	static PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = VK_NULL_HANDLE;
	static PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = VK_NULL_HANDLE;
	static PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT = VK_NULL_HANDLE;
	static PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = VK_NULL_HANDLE;
	static PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabelEXT = VK_NULL_HANDLE;
	static PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabelEXT = VK_NULL_HANDLE;

	VkResult CreateDebugUtilsEXT(VkInstance instance)
	{
		vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
 		vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
		vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT");
 		vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
		vkQueueBeginDebugUtilsLabelEXT = (PFN_vkQueueBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkQueueBeginDebugUtilsLabelEXT");
		vkQueueEndDebugUtilsLabelEXT = (PFN_vkQueueEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkQueueEndDebugUtilsLabelEXT");

		if (vkSetDebugUtilsObjectNameEXT == nullptr)
			return VK_ERROR_EXTENSION_NOT_PRESENT;

		return VK_SUCCESS;
	}

	namespace VKUtils {

		void SetDebugUtilsObjectName(VkDevice device, VkObjectType objectType, const std::string& debugName, void* object)
		{
			if (vkSetDebugUtilsObjectNameEXT)
			{
				VkDebugUtilsObjectNameInfoEXT debugUtilsNameInfo{};
				debugUtilsNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
				debugUtilsNameInfo.objectType = objectType;
				debugUtilsNameInfo.objectHandle = (uint64_t)object;
				debugUtilsNameInfo.pObjectName = debugName.c_str();

				vkSetDebugUtilsObjectNameEXT(device, &debugUtilsNameInfo);
			}
		}

		void SetCommandBufferLabel(VkCommandBuffer cmdBuffer, const char* labelName, float labelColor[])
		{
			if (vkCmdBeginDebugUtilsLabelEXT)
			{
				VkDebugUtilsLabelEXT debugLabelEXT{};
				debugLabelEXT.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
				debugLabelEXT.pLabelName = labelName;
				debugLabelEXT.color[0] = labelColor[0];
				debugLabelEXT.color[1] = labelColor[1];
				debugLabelEXT.color[2] = labelColor[2];
				debugLabelEXT.color[3] = labelColor[3];

				vkCmdBeginDebugUtilsLabelEXT(cmdBuffer, &debugLabelEXT);
			}
		}

		void EndCommandBufferLabel(VkCommandBuffer cmdBuffer)
		{
			if (vkCmdEndDebugUtilsLabelEXT)
				vkCmdEndDebugUtilsLabelEXT(cmdBuffer);
		}

		void SetQueueLabel(VkQueue queue, const char* labelName, float labelColor[])
		{
			if (vkQueueBeginDebugUtilsLabelEXT)
			{
				VkDebugUtilsLabelEXT debugLabelEXT{};
				debugLabelEXT.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
				debugLabelEXT.pLabelName = labelName;
				debugLabelEXT.color[0] = labelColor[0];
				debugLabelEXT.color[1] = labelColor[1];
				debugLabelEXT.color[2] = labelColor[2];
				debugLabelEXT.color[3] = labelColor[3];

				vkQueueBeginDebugUtilsLabelEXT(queue, &debugLabelEXT);
			}
		}

		void EndQueueLabel(VkQueue queue)
		{
			if (vkQueueEndDebugUtilsLabelEXT)
				vkQueueEndDebugUtilsLabelEXT(queue);
		}

	}

	namespace Utils {

		std::string VkObjectTypeToString(VkObjectType objectType)
		{
			switch (objectType)
			{
			case VK_OBJECT_TYPE_UNKNOWN:			   return "Unknown";
			case VK_OBJECT_TYPE_BUFFER:				   return "Buffer";
			case VK_OBJECT_TYPE_COMMAND_BUFFER:		   return "Command Buffer";
			case VK_OBJECT_TYPE_COMMAND_POOL:		   return "Command Pool";
			case VK_OBJECT_TYPE_INSTANCE:			   return "Instance";
			case VK_OBJECT_TYPE_DEVICE:				   return "Device";
			case VK_OBJECT_TYPE_DEVICE_MEMORY:		   return "Device Memory";
			case VK_OBJECT_TYPE_DESCRIPTOR_POOL:	   return "Descriptor Pool";
			case VK_OBJECT_TYPE_DESCRIPTOR_SET:		   return "Descriptor Set";
			case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT: return "Descriptor Set Layout";
			case VK_OBJECT_TYPE_IMAGE:				   return "Image";
			case VK_OBJECT_TYPE_IMAGE_VIEW:			   return "Image View";
			case VK_OBJECT_TYPE_PIPELINE:			   return "Pipeline";
			case VK_OBJECT_TYPE_PIPELINE_LAYOUT:	   return "Pipeline Layout";
			case VK_OBJECT_TYPE_RENDER_PASS:		   return "Render Pass";
			case VK_OBJECT_TYPE_SAMPLER:			   return "Image Sampler";
			case VK_OBJECT_TYPE_FRAMEBUFFER:		   return "Framebuffer";
			case VK_OBJECT_TYPE_SHADER_MODULE:		   return "Shader Module";
			case VK_OBJECT_TYPE_QUEUE:				   return "Queue";
			case VK_OBJECT_TYPE_QUERY_POOL:			   return "Query Pool";
			case VK_OBJECT_TYPE_SWAPCHAIN_KHR:		   return "Swapchain";
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
			labels = std::format("\tLabels({}): \n", pCallbackData->cmdBufLabelCount);
			for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; ++i)
			{
				const auto& label = pCallbackData->pCmdBufLabels[i];
				const std::string colorStr = std::format("[ {0}, {1}, {2}, {3} ]", label.color[0], label.color[1], label.color[2], label.color[3]);
				labels.append(std::format("\t\t- Command Buffer Label[{0}]: name: {1}, color: {2}\n", i, label.pLabelName ? label.pLabelName : "NULL", colorStr));
			}
		}

		if (pCallbackData->objectCount)
		{
			objects = std::format("\tObjects({}): \n", pCallbackData->objectCount);
			for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
			{
				const auto& object = pCallbackData->pObjects[i];
				objects.append(std::format("\t\t- Object[{0}]: name: {1}, type: {2}, handle: {3:#x}\n", i, object.pObjectName ? object.pObjectName : "NULL", Utils::VkObjectTypeToString(object.objectType), object.objectHandle));
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
		appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 4, 0);
		appInfo.pEngineName = "VulkanEngine";
		appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 4, 0);
		appInfo.apiVersion = VK_API_VERSION_1_4;

		auto extensions = GetRequiredExtensions();

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (m_EnableValidation)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = &debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance), "Failed to Create Vulkan Instance!");

		HasGLFWRequiredInstanceExtensions();
	}

	void VulkanContext::SetupDebugMessenger()
	{
		if (!m_EnableValidation)
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);
		VK_CHECK_RESULT(CreateDebugUtilsMessengerEXT(m_VulkanInstance, &createInfo, nullptr, &m_DebugMessenger), "Failed to Setup Debug Messenger!");
		VK_CHECK_RESULT(CreateDebugUtilsEXT(m_VulkanInstance), "Failed to Set Debug Utils!");
	}

	void VulkanContext::CreateSurface()
	{
		VK_CHECK_RESULT(glfwCreateWindowSurface(m_VulkanInstance, (GLFWwindow*)m_Window->GetNativeWindow(), nullptr, &m_VulkanSurface),
			"Failed to Create Window Surface!");
	}

	void VulkanContext::CreateVulkanMemoryAllocator()
	{
		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocatorInfo{};
		allocatorInfo.device = m_Device->GetVulkanDevice();
		allocatorInfo.instance = m_VulkanInstance;
		allocatorInfo.physicalDevice = m_Device->GetPhysicalDevice();
		allocatorInfo.pVulkanFunctions = &vulkanFunctions;
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;
		allocatorInfo.pAllocationCallbacks = nullptr;
		allocatorInfo.pDeviceMemoryCallbacks = nullptr;
		allocatorInfo.pHeapSizeLimit = nullptr;
		allocatorInfo.pTypeExternalMemoryHandleTypes = nullptr;

		vmaCreateAllocator(&allocatorInfo, &m_VulkanMemoryAllocator);
	}

	bool VulkanContext::IsDeviceSuitable(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices = m_Device->FindQueueFamilies(device);

		bool swapChainAdequate = false, extensionsSupported = CheckDeviceExtensionSupport(device);
		if (extensionsSupported)
		{
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures{};
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
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_VulkanSurface, &details.Capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VulkanSurface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			details.Formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VulkanSurface, &formatCount, details.Formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_VulkanSurface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			details.PresentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_VulkanSurface, &presentModeCount, details.PresentModes.data());
		}

		return details;
	}

	void VulkanContext::Destroy()
	{
		vmaDestroyAllocator(m_VulkanMemoryAllocator);
		m_Device->Destroy();

		if (m_EnableValidation)
			DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);

		vkDestroySurfaceKHR(m_VulkanInstance, m_VulkanSurface, nullptr);
		vkDestroyInstance(m_VulkanInstance, nullptr);
	}

}
