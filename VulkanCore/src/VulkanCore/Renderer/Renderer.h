#pragma once
#include "VulkanRenderer.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "Platform/Vulkan/VulkanRenderCommandBuffer.h"
#include "VulkanCore/Mesh/Mesh.h"
#include "VulkanCore/Core/Shader.h"
#include "VulkanCore/Renderer/RenderThread.h"

namespace VulkanCore {

	class VulkanRenderer;

	struct RendererConfig
	{
		static const uint32_t FramesInFlight = 3;
	};

	class Renderer
	{
	public:
		static void SetRendererAPI(VulkanRenderer* vkRenderer);
		static int GetCurrentFrameIndex();
		static int RT_GetCurrentFrameIndex();
		static RendererConfig GetConfig();
		static void BeginRenderPass(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, std::shared_ptr<VulkanRenderPass> renderPass);
		static void EndRenderPass(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, std::shared_ptr<VulkanRenderPass> renderPass);
		static void BuildShaders();
		static void DestroyShaders();

		static void RenderSkybox(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, std::shared_ptr<VulkanPipeline> pipeline, std::shared_ptr<VulkanVertexBuffer> skyboxVB, const std::vector<VkDescriptorSet>& descriptorSet, void* pcData = nullptr);
		static void BeginTimestampsQuery(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer);
		static void EndTimestampsQuery(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer);
		static void BeginGPUPerfMarker(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, const std::string& name);
		static void EndGPUPerfMarker(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer);

		static std::shared_ptr<VulkanTexture> GetWhiteTexture(ImageFormat format = ImageFormat::RGBA8_SRGB);
		static void SubmitFullscreenQuad(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, const std::shared_ptr<VulkanPipeline>& pipeline, const std::vector<VkDescriptorSet>& descriptorSet);
		static void RenderMesh(std::shared_ptr<Mesh> mesh);

		static std::shared_ptr<Shader> GetShader(const std::string& name)
		{
			if (!m_Shaders.contains(name))
				return nullptr;

			return m_Shaders[name];
		}
		
		template<typename FuncT>
		static void Submit(FuncT&& func)
		{
			RenderThread::SubmitToThread(func);
		}

#if USE_DELETION_QUEUE
		template<typename FuncT>
		static void SubmitResourceFree(FuncT&& func)
		{
			RenderThread::SubmitToDeletion(func);
		}
#endif

		static void Init();
		static void WaitAndRender();
		static void WaitAndExecute();
	private:
		static std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
		static VulkanRenderer* s_Renderer;
		static RendererConfig s_RendererConfig;
	};

}