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

		VkFramebuffer GetFramebuffer(int index) const { return m_SwapChainFramebuffers[index]; }
		VkRenderPass GetRenderPass() const { return m_RenderPass; }
		VkImageView GetImageView(int index) const { return m_SwapChainImageViews[index]; }
		VkImage GetSwapChainImage(int index) const { return m_SwapChainImages[index]; }
		size_t GetImageCount() const { return m_SwapChainImages.size(); }
		VkFormat GetSwapChainImageFormat() const { return m_SwapChainImageFormat; }
		VkExtent2D GetSwapChainExtent() const { return m_SwapChainExtent; }
		uint32_t GetWidth() const { return m_SwapChainExtent.width; }
		uint32_t GetHeight() const { return m_SwapChainExtent.height; }

		float ExtentAspectRatio() { return static_cast<float>(m_SwapChainExtent.width) / static_cast<float>(m_SwapChainExtent.height); }
		VkFormat FindDepthFormat();

		VkResult AcquireNextImage(uint32_t* imageIndex);
		VkResult SubmitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);
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
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	private:
		VkFormat m_SwapChainImageFormat;
		VkFormat m_SwapChainDepthFormat;
		VkExtent2D m_SwapChainExtent;

		std::vector<VkFramebuffer> m_SwapChainFramebuffers;
		VkRenderPass m_RenderPass;

		// Only for MSAA
		std::vector<VkImage> m_ColorImages;
		std::vector<VmaAllocation> m_ColorImageMemories;
		std::vector<VkImageView> m_ColorImageViews;

		// Required for Depth
		std::vector<VkImage> m_DepthImages;
		std::vector<VmaAllocation> m_DepthImageMemories;
		std::vector<VkImageView> m_DepthImageViews;

		// Required to receive images from Swap Chain
		std::vector<VkImage> m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;

		VkExtent2D m_WindowExtent;

		VkSwapchainKHR m_SwapChain;
		std::shared_ptr<VulkanSwapChain> m_OldSwapChain;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
		std::vector<VkFence> m_ImagesInFlight;
		size_t m_CurrentFrame = 0;

		static VulkanSwapChain* s_Instance;
	};

}
