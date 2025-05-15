#include "vulkanpch.h"
#include "VulkanDevice.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Application.h"
#include "VulkanContext.h"

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

	VulkanDevice::VulkanDevice()
	{
		Init();
	}

	void VulkanDevice::Init()
	{
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateCommandPools();
	}

	void VulkanDevice::Destroy()
	{
		vkDestroyCommandPool(m_LogicalDevice, m_CommandPool, nullptr);
		vkDestroyCommandPool(m_LogicalDevice, m_RTCommandPool, nullptr);
		vkDestroyCommandPool(m_LogicalDevice, m_ComputeCommandPool, nullptr);
		vkDestroyDevice(m_LogicalDevice, nullptr);
	}

	VulkanDevice::~VulkanDevice()
	{
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
		return (VkFormat)0;
	}

	bool VulkanDevice::IsExtensionSupported(const char* extensionName)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, extensions.data());

		for (const auto& extension : extensions)
		{
			if (strcmp(extension.extensionName, extensionName) == 0)
				return true;
		}

		return false;
	}

	bool VulkanDevice::IsInDebugMode() const
	{
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> layers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

		for (const auto& layer : layers)
		{
			if (strcmp(layer.layerName, "VK_LAYER_RENDERDOC_Capture") == 0)
				return true; // RenderDoc layer is active
		}

		return false; // RenderDoc layer not found
	}

	VkCommandBuffer VulkanDevice::GetCommandBuffer(bool compute)
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = compute ? m_ComputeCommandPool : m_CommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		return commandBuffer;
	}

	void VulkanDevice::FlushCommandBuffer(VkCommandBuffer commandBuffer, bool compute)
	{
		const uint64_t DEFAULT_FENCE_TIMEOUT = 100000000000;

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = 0;

		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(m_LogicalDevice, &fenceCreateInfo, nullptr, &fence), "Failed to Create Fence!");

		// Submit to Queue
		VK_CHECK_RESULT(vkQueueSubmit(compute ? m_ComputeQueue : m_GraphicsQueue, 1, &submitInfo, fence), "Failed to Submit to Queue!");
		// Wait for the fence to signal
		VK_CHECK_RESULT(vkWaitForFences(m_LogicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT), "Failed to Wait for Fence to signal!");

		vkDestroyFence(m_LogicalDevice, fence, nullptr);
		vkFreeCommandBuffers(m_LogicalDevice, compute ? m_ComputeCommandPool : m_CommandPool, 1, &commandBuffer);
	}

	void VulkanDevice::PickPhysicalDevice()
	{
		auto context = VulkanContext::GetCurrentContext();

		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(context->m_VulkanInstance, &deviceCount, nullptr);
		VK_CORE_ASSERT(deviceCount != 0, "Failed to find GPUs with Vulkan Support!");

		VK_CORE_INFO("Device Count: {0}", deviceCount);
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(context->m_VulkanInstance, &deviceCount, devices.data());

		for (const auto& device : devices)
		{
			vkGetPhysicalDeviceProperties(device, &m_DeviceProperties);

			if (m_DeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && context->IsDeviceSuitable(device))
			{
				m_PhysicalDevice = device;
				break;
			}
		}

		VK_CORE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Failed to find a Suitable GPU!");
		VK_CORE_INFO("Physical Device: {0}", m_DeviceProperties.deviceName);

		auto sampleCount = m_DeviceProperties.limits.framebufferColorSampleCounts & m_DeviceProperties.limits.framebufferDepthSampleCounts;
		m_MSAASamples = VK_SAMPLE_COUNT_4_BIT; // TODO: Get this through some function
	}

	void VulkanDevice::CreateLogicalDevice()
	{
		auto context = VulkanContext::GetCurrentContext();
		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.GraphicsFamily, indices.ComputeFamily, indices.PresentFamily };

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
		deviceFeatures.tessellationShader = VK_TRUE;
		deviceFeatures.fillModeNonSolid = VK_TRUE;
		deviceFeatures.shaderInt64 = VK_TRUE;
		deviceFeatures.multiViewport = VK_TRUE;
		deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;
		deviceFeatures.depthClamp = VK_TRUE;

		// Examples: https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
		VkPhysicalDeviceSynchronization2Features physicalDeviceSynchronization2Features{};
		physicalDeviceSynchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
		physicalDeviceSynchronization2Features.synchronization2 = VK_TRUE;

		VkPhysicalDeviceFeatures2 physicalDeviceFeatures{};
		physicalDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		physicalDeviceFeatures.features = deviceFeatures;
		physicalDeviceFeatures.pNext = &physicalDeviceSynchronization2Features;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(context->m_DeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = context->m_DeviceExtensions.data();
		createInfo.pEnabledFeatures = nullptr;
		createInfo.pNext = &physicalDeviceFeatures;

		VK_CHECK_RESULT(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice), "Failed to Create Logical Device!");

		vkGetDeviceQueue(m_LogicalDevice, indices.GraphicsFamily, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_LogicalDevice, indices.ComputeFamily, 0, &m_ComputeQueue);
		vkGetDeviceQueue(m_LogicalDevice, indices.PresentFamily, 0, &m_PresentQueue);
	}

	void VulkanDevice::CreateCommandPools()
	{
		QueueFamilyIndices queueFamilyIndices = FindPhysicalQueueFamilies();

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VK_CHECK_RESULT(vkCreateCommandPool(m_LogicalDevice, &poolInfo, nullptr, &m_RTCommandPool), "Failed to Create Command Pool!");
		VK_CHECK_RESULT(vkCreateCommandPool(m_LogicalDevice, &poolInfo, nullptr, &m_CommandPool), "Failed to Create Command Pool!");

		poolInfo.queueFamilyIndex = queueFamilyIndices.ComputeFamily;
		VK_CHECK_RESULT(vkCreateCommandPool(m_LogicalDevice, &poolInfo, nullptr, &m_ComputeCommandPool), "Failed to Create Compute Command Pool!");

		VKUtils::SetDebugUtilsObjectName(m_LogicalDevice, VK_OBJECT_TYPE_COMMAND_POOL, "Default Command Pool", m_CommandPool);
		VKUtils::SetDebugUtilsObjectName(m_LogicalDevice, VK_OBJECT_TYPE_COMMAND_POOL, "Render Thread Command Pool", m_RTCommandPool);
		VKUtils::SetDebugUtilsObjectName(m_LogicalDevice, VK_OBJECT_TYPE_COMMAND_POOL, "Compute Command Pool", m_ComputeCommandPool);
	}

	QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices{};

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		const auto vulkanSurface = VulkanContext::GetCurrentContext()->m_VulkanSurface;

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.GraphicsFamily = i;
				indices.GraphicsFamilyHasValue = true;
			}

			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				indices.ComputeFamily = i;
				indices.ComputeFamilyHasValue = indices.GraphicsFamily != indices.ComputeFamily;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vulkanSurface, &presentSupport);

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

}
