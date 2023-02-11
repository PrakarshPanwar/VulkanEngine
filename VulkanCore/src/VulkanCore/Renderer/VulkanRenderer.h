#pragma once
#include "Platform/Windows/WindowsWindow.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "Platform/Vulkan/VulkanTexture.h"
#include "Platform/Vulkan/VulkanSwapChain.h"
#include "Platform/Vulkan/VulkanRenderCommandBuffer.h"
#include "VulkanCore/Mesh/Mesh.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Components.h"

namespace VulkanCore {

	struct RendererStats
	{
		uint32_t DrawCalls;
		uint32_t InstanceCount;
	};

	class VulkanRenderer
	{
	public:
		VulkanRenderer(std::shared_ptr<WindowsWindow> window);
		~VulkanRenderer();

		void Init();

		VkCommandBuffer BeginFrame();
		void EndFrame();
		VkCommandBuffer BeginScene();
		void EndScene();
		void BeginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void EndSwapChainRenderPass(VkCommandBuffer commandBuffer);
		inline float GetAspectRatio() const { return m_SwapChain->ExtentAspectRatio(); }

		inline bool IsFrameInProgress() const { return IsFrameStarted; }
		inline VkCommandBuffer GetCurrentCommandBuffer() const
		{
			VK_CORE_ASSERT(IsFrameStarted, "Cannot get Command Buffer when frame is not in progress!");
			return m_CommandBuffers[m_CurrentFrameIndex];
		}

		static std::tuple<std::shared_ptr<VulkanTextureCube>, std::shared_ptr<VulkanTextureCube>> CreateEnviromentMap(const std::string& filepath);
		static std::shared_ptr<VulkanImage> CreateBRDFTexture();
		static void CopyVulkanImage(VkCommandBuffer cmdBuf, const VulkanImage& sourceImage, const VulkanImage& destImage);
		static void BlitVulkanImage(VkCommandBuffer cmdBuf, const VulkanImage& image);
		static void RenderMesh(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, std::shared_ptr<Mesh> mesh, std::shared_ptr<VulkanVertexBuffer> transformBuffer, const std::vector<TransformData>& transformData, uint32_t instanceCount);
		static RendererStats GetRendererStats() { return s_Data; }
		static void ResetStats();

		inline VkRenderPass GetSwapChainRenderPass() const { return m_SwapChain->GetRenderPass(); }
		inline int GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }
		inline VkQueryPool GetPerfQueryPool() const { return m_QueryPool; }

		void RecreateSwapChain();
		void FinalQueueSubmit();
		void FinalQueueSubmit(const std::vector<VkCommandBuffer>& cmdBuffers);
		static VulkanRenderer* Get() { return s_Instance; }
	private:
		void CreateCommandBuffers();
		void FreeCommandBuffers();
		void CreateQueryPool();
		void FreeQueryPool();
	private:
		std::shared_ptr<WindowsWindow> m_Window;
		std::unique_ptr<VulkanSwapChain> m_SwapChain;

		std::vector<VkCommandBuffer> m_CommandBuffers;
		std::vector<VkCommandBuffer> m_SecondaryCommandBuffers;
		std::array<VkCommandBuffer, 2> m_ExecuteCommandBuffers;

		uint32_t m_CurrentImageIndex;
		int m_CurrentFrameIndex = 0;
		bool IsFrameStarted = false;

		VkQueryPool m_QueryPool;
		const uint32_t m_QueryCount = 10; // TODO: This number could change in future

		static RendererStats s_Data;
		static VulkanRenderer* s_Instance;
	};

}