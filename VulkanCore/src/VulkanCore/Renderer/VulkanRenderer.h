#pragma once
#include "Platform/Windows/WindowsWindow.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "Platform/Vulkan/VulkanSwapChain.h"
#include "Platform/Vulkan/VulkanModel.h"
#include "Platform/Vulkan/VulkanGameObject.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

namespace VulkanCore {

	class VulkanRenderer
	{
	public:
		VulkanRenderer(WindowsWindow& appwindow, VulkanDevice& device);
		~VulkanRenderer();

		void Init();

		VkCommandBuffer BeginFrame();
		void EndFrame();
		void BeginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void EndSwapChainRenderPass(VkCommandBuffer commandBuffer);
		inline float GetAspectRatio() const { return m_SwapChain->ExtentAspectRatio(); }

		inline bool IsFrameInProgress() const { return IsFrameStarted; }
		inline VkCommandBuffer GetCurrentCommandBuffer() const
		{
			VK_CORE_ASSERT(IsFrameStarted, "Cannot get Command Buffer when frame is not in progress!");
			return m_CommandBuffers[m_CurrentFrameIndex];
		}

		inline VkRenderPass GetSwapChainRenderPass() const { return m_SwapChain->GetRenderPass(); }
		inline int GetFrameIndex() const 
		{
			VK_CORE_ASSERT(IsFrameStarted, "Cannot get Frame Index when frame is not in progress!");
			return m_CurrentFrameIndex;
		}

		void RecreateSwapChain();
		static VulkanRenderer* Get() { return s_Instance; }
	private:
		void CreateCommandBuffers();
		void FreeCommandBuffers();
	private:
		WindowsWindow& m_Window;
		VulkanDevice& m_VulkanDevice;
		std::unique_ptr<VulkanSwapChain> m_SwapChain;

		std::vector<VkCommandBuffer> m_CommandBuffers;
		uint32_t m_CurrentImageIndex;
		int m_CurrentFrameIndex = 0;
		bool IsFrameStarted = false;

		static VulkanRenderer* s_Instance;
	};

}