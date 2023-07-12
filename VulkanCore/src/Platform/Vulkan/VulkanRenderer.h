#pragma once
#include "Platform/Windows/WindowsWindow.h"
#include "Platform/Vulkan/VulkanDescriptor.h"
#include "Platform/Vulkan/VulkanSwapChain.h"
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

		static std::tuple<std::shared_ptr<TextureCube>, std::shared_ptr<TextureCube>> CreateEnviromentMap(const std::string& filepath);
		static std::shared_ptr<Image2D> CreateBRDFTexture();
		void BeginRenderPass(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, std::shared_ptr<RenderPass> renderPass);
		void EndRenderPass(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, std::shared_ptr<RenderPass> renderPass);
		void RenderSkybox(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<VertexBuffer>& skyboxVB, const std::shared_ptr<Material>& skyboxMaterial, void* pcData = nullptr);
		void BeginTimestampsQuery(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer);
		void EndTimestampsQuery(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer);
		void BeginGPUPerfMarker(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::string& name, DebugLabelColor labelColor = DebugLabelColor::None);
		void EndGPUPerfMarker(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer);
		void CopyVulkanImage(const std::shared_ptr<RenderCommandBuffer>& commandBuffer, const std::shared_ptr<Image2D>& sourceImage, const std::shared_ptr<Image2D>& destImage);
		void BlitVulkanImage(const std::shared_ptr<RenderCommandBuffer>& commandBuffer, const std::shared_ptr<Image2D>& image);
		void RenderMesh(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, uint32_t submeshIndex, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<VertexBuffer>& transformBuffer, const std::vector<TransformData>& transformData, uint32_t instanceCount);
		void RenderTransparentMesh(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, uint32_t submeshIndex, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<VertexBuffer>& transformBuffer, const std::vector<TransformData>& transformData, uint32_t instanceCount);
		void ResetStats();

		inline VkRenderPass GetSwapChainRenderPass() const { return m_SwapChain->GetRenderPass(); }
		inline int GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }
		inline std::shared_ptr<VulkanDescriptorPool> GetDescriptorPool() const { return m_GlobalDescriptorPool; }

		void RecreateSwapChain();
		void FinalQueueSubmit(const std::vector<VkCommandBuffer>& cmdBuffers);
		void SubmitAndPresent();
		static RendererStats GetRendererStats() { return s_Data; }
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