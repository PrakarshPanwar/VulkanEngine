#pragma once
#include "VulkanContext.h"

namespace VulkanCore {

	class VulkanSwapChain
	{
	public:
		VulkanSwapChain(VkExtent2D windowExtent);
		VulkanSwapChain(VkExtent2D windowExtent, std::shared_ptr<VulkanSwapChain> prev);
		~VulkanSwapChain();

		static VulkanSwapChain* GetSwapChain() { return s_Instance; }

		VkFramebuffer GetFramebuffer(int index) const { return m_SCFramebuffers[index]; }
		VkRenderPass GetRenderPass() const { return m_SCRenderPass; }
		VkImageView GetImageView(int index) const { return m_SCImageViews[index]; }
		VkImage GetSwapChainImage(int index) const { return m_SCImages[index]; }
		size_t GetImageCount() const { return m_SCImages.size(); }
		VkFormat GetSwapChainImageFormat() const { return m_SCImageFormat; }
		VkExtent2D GetSwapChainExtent() const { return m_SCExtent; }
		uint32_t GetWidth() const { return m_SCExtent.width; }
		uint32_t GetHeight() const { return m_SCExtent.height; }

		float ExtentAspectRatio() { return static_cast<float>(m_SCExtent.width) / static_cast<float>(m_SCExtent.height); }
		VkFormat FindDepthFormat();

		VkResult AcquireNextImage(uint32_t* imageIndex);
		//VkResult SubmitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);
		VkResult SubmitCommandBuffers(const std::vector<VkCommandBuffer>& buffers, uint32_t* imageIndex);

		bool CompareSwapFormats(const VulkanSwapChain& swapChain) const;
	private:
		void Init();
		void CreateSwapChain();
		void CreateImageViews();
		void CreateColorResources(); // Only for MSAA
		void CreateDepthResources();
		void CreateRenderPass();
		void CreateFramebuffers();
		void CreateSyncObjects();

		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, VkPresentModeKHR requiredPresentMode);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	private:
		VkFormat m_SCImageFormat, m_SCDepthFormat;
		VkExtent2D m_SCExtent, m_WindowExtent;

		std::vector<VkFramebuffer> m_SCFramebuffers;
		VkRenderPass m_SCRenderPass;

		// Only for MSAA
		std::vector<VkImage> m_ColorImages, m_DepthImages;
		std::vector<VmaAllocation> m_ColorImageAllocations, m_DepthImageAllocations;
		std::vector<VkImageView> m_ColorImageViews, m_DepthImageViews;

		// Required to receive images from Swap Chain
		std::vector<VkImage> m_SCImages;
		std::vector<VkImageView> m_SCImageViews;

		VkSwapchainKHR m_SwapChain;
		std::shared_ptr<VulkanSwapChain> m_OldSwapChain;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores, m_ReadyToPresentSemaphores;
		std::vector<VkFence> m_InFlightFences, m_ImagesInFlight;
		size_t m_CurrentFrame = 0;

		static VulkanSwapChain* s_Instance;
	};

}
