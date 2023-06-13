#pragma once
#include "Platform/Windows/WindowsWindow.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "Platform/Vulkan/VulkanTexture.h"
#include "Platform/Vulkan/VulkanDescriptor.h"
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

		void BeginFrame();
		void EndFrame();
		void BeginScene();
		void EndScene();
		void BeginSwapChainRenderPass();
		void EndSwapChainRenderPass();
		inline float GetAspectRatio() const { return m_SwapChain->ExtentAspectRatio(); }

		inline bool IsFrameInProgress() const { return IsFrameStarted; }
		inline std::shared_ptr<VulkanRenderCommandBuffer> GetRendererCommandBuffer() const { return m_CommandBuffer; }
		inline VkCommandBuffer GetCurrentCommandBuffer() const
		{
			VK_CORE_ASSERT(IsFrameStarted, "Cannot get Command Buffer when frame is not in progress!");
			return m_CommandBuffer->GetActiveCommandBuffer();
		}

		static std::tuple<std::shared_ptr<VulkanTextureCube>, std::shared_ptr<VulkanTextureCube>> CreateEnviromentMap(const std::string& filepath);
		static std::shared_ptr<VulkanImage> CreateBRDFTexture();
		static void CopyVulkanImage(std::shared_ptr<VulkanRenderCommandBuffer> commandBuffer, const std::shared_ptr<VulkanImage>& sourceImage, const std::shared_ptr<VulkanImage>& destImage);
		static void BlitVulkanImage(std::shared_ptr<VulkanRenderCommandBuffer> commandBuffer, const std::shared_ptr<VulkanImage>& image);
		static void RenderMesh(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, uint32_t submeshIndex, std::shared_ptr<VulkanPipeline> pipeline, std::shared_ptr<VulkanVertexBuffer> transformBuffer, const std::vector<TransformData>& transformData, uint32_t instanceCount);
		static RendererStats GetRendererStats() { return s_Data; }
		static void ResetStats();

		inline VkRenderPass GetSwapChainRenderPass() const { return m_SwapChain->GetRenderPass(); }
		inline int GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }
		inline std::shared_ptr<VulkanDescriptorPool> GetDescriptorPool() const { return m_GlobalDescriptorPool; }

		void RecreateSwapChain();
		void FinalQueueSubmit(const std::vector<VkCommandBuffer>& cmdBuffers);
		void SubmitAndPresent();
		static VulkanRenderer* Get() { return s_Instance; }
	private:
		void CreateCommandBuffers();
		void InitDescriptorPool();
	private:
		std::shared_ptr<WindowsWindow> m_Window;
		std::unique_ptr<VulkanSwapChain> m_SwapChain;
		std::shared_ptr<VulkanDescriptorPool> m_GlobalDescriptorPool;

		std::shared_ptr<VulkanRenderCommandBuffer> m_CommandBuffer;

		uint32_t m_CurrentImageIndex;
		int m_CurrentFrameIndex = 0;
		bool IsFrameStarted = false;

		static RendererStats s_Data;
		static VulkanRenderer* s_Instance;
	};

}