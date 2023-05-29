#pragma once
#include "VulkanRenderer.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "Platform/Vulkan/VulkanRenderCommandBuffer.h"
#include "VulkanCore/Mesh/Mesh.h"
#include "VulkanCore/Core/Shader.h"
#include "VulkanCore/Renderer/RenderThread.h"

namespace VulkanCore {

	class VulkanRenderer;
	class VulkanMaterial;

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
		static void ShutDown();

		static void RenderSkybox(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, std::shared_ptr<VulkanPipeline> pipeline, std::shared_ptr<VulkanVertexBuffer> skyboxVB, const std::shared_ptr<VulkanMaterial>& skyboxMaterial, void* pcData = nullptr);
		static void BeginTimestampsQuery(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer);
		static void EndTimestampsQuery(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer);
		static void BeginGPUPerfMarker(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, const std::string& name);
		static void EndGPUPerfMarker(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer);

		static std::shared_ptr<VulkanTexture> GetWhiteTexture(ImageFormat format = ImageFormat::RGBA8_SRGB);
		static void SubmitFullscreenQuad(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, const std::shared_ptr<VulkanPipeline>& pipeline, const std::shared_ptr<VulkanMaterial>& shaderMaterial);

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

		template<typename FuncT>
		static void SubmitResourceFree(FuncT&& func)
		{
			RenderThread::SubmitToDeletion(func);
		}

		static void Init();
		static void WaitAndRender();
		static void WaitAndExecute();
	private:
		static std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
		static VulkanRenderer* s_Renderer;
		static RendererConfig s_RendererConfig;
	};

}