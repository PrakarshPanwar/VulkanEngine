#pragma once
#include "Platform/Vulkan/VulkanRenderer.h"
#include "Platform/Vulkan/VulkanRenderPass.h"
#include "VulkanCore/Mesh/Mesh.h"
#include "VulkanCore/Renderer/Shader.h"
#include "VulkanCore/Renderer/RenderThread.h"

namespace VulkanCore {

	class VulkanRenderer;
	class VulkanMaterial;

	// More Colors can be added in future
	enum class DebugLabelColor
	{
		None,
		Grey,
		Red,
		Blue,
		Gold,
		Orange,
		Pink,
		Aqua,
		Green
	};

	struct RendererConfig
	{
		static constexpr uint32_t FramesInFlight = 3;
	};

	class Renderer
	{
	public:
		static void BeginRenderPass(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, std::shared_ptr<RenderPass> renderPass);
		static void EndRenderPass(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, std::shared_ptr<RenderPass> renderPass);
		static void RenderSkybox(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<VertexBuffer>& skyboxVB, const std::shared_ptr<Material>& skyboxMaterial, void* pcData = nullptr);
		static void BeginTimestampsQuery(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer);
		static void EndTimestampsQuery(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer);
		static void BeginGPUPerfMarker(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::string& name, DebugLabelColor labelColor = DebugLabelColor::None);
		static void EndGPUPerfMarker(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer);

		static std::shared_ptr<Texture2D> GetWhiteTexture(ImageFormat format = ImageFormat::RGBA8_SRGB);
		static void SubmitFullscreenQuad(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Material>& shaderMaterial);

		static int GetCurrentFrameIndex();
		static int RT_GetCurrentFrameIndex();
		static RendererConfig GetConfig();
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
	public:
		static void Init();
		static void ShutDown();
		static void SetRendererAPI(VulkanRenderer* vkRenderer);
		static void BuildShaders();
		static void WaitAndRender();
		static void WaitAndExecute();
	private:
		static std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
		static VulkanRenderer* s_Renderer;
		static RendererConfig s_RendererConfig;
	};

}