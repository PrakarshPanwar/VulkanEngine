#pragma once
#include "Platform/Windows/WindowsWindow.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "Platform/Vulkan/VulkanSwapChain.h"
#include "Platform/Vulkan/VulkanMesh.h"

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
		VkCommandBuffer BeginSceneFrame();
		void EndSceneFrame();
		void BeginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void EndSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void BeginSceneRenderPass(VkCommandBuffer commandBuffer);
		void EndSceneRenderPass(VkCommandBuffer commandBuffer);
		inline float GetAspectRatio() const { return m_SwapChain->ExtentAspectRatio(); }

		inline bool IsFrameInProgress() const { return IsFrameStarted; }
		inline VkCommandBuffer GetCurrentCommandBuffer() const
		{
			VK_CORE_ASSERT(IsFrameStarted, "Cannot get Command Buffer when frame is not in progress!");
			return m_CommandBuffers[m_CurrentFrameIndex];
		}

		inline VkRenderPass GetSwapChainRenderPass() const { return m_SwapChain->GetRenderPass(); }
		inline int GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }
		inline VkQueryPool GetPerfQueryPool() const { return m_QueryPool; }

		void RecreateSwapChain();
		void FinalQueueSubmit();
		static VulkanRenderer* Get() { return s_Instance; }
	private:
		void CreateCommandBuffers();
		void FreeCommandBuffers();
		void CreateQueryPool();
		void FreeQueryPool();
	private:
		WindowsWindow& m_Window;
		VulkanDevice& m_VulkanDevice;
		std::unique_ptr<VulkanSwapChain> m_SwapChain;

		std::vector<VkCommandBuffer> m_CommandBuffers;
		VkQueryPool m_QueryPool;
		uint32_t m_CurrentImageIndex;
		int m_CurrentFrameIndex = 0;
		bool IsFrameStarted = false;

		static VulkanRenderer* s_Instance;
	};

}