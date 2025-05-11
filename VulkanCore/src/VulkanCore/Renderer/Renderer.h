#pragma once
#include "VulkanCore/Mesh/Mesh.h"
#include "VulkanCore/Core/Components.h"
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
		static void RenderSkybox(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Material>& skyboxMaterial, void* pcData = nullptr);
		static void BeginTimestampsQuery(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer);
		static void EndTimestampsQuery(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer);
		static void BeginGPUPerfMarker(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::string& name, DebugLabelColor labelColor = DebugLabelColor::None);
		static void EndGPUPerfMarker(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer);
		static void BindPipeline(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Material>& material);
		static void CopyVulkanImage(const std::shared_ptr<RenderCommandBuffer>& commandBuffer, const std::shared_ptr<Image2D>& sourceImage, const std::shared_ptr<Image2D>& destImage);
		static void BlitVulkanImage(const std::shared_ptr<RenderCommandBuffer>& commandBuffer, const std::shared_ptr<Image2D>& image);
		static void RenderMesh(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, uint32_t submeshIndex, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<VertexBuffer>& transformBuffer, const std::vector<TransformData>& transformData, uint32_t instanceCount);
		static void RenderSelectedMesh(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Mesh>& mesh, uint32_t submeshIndex, const std::shared_ptr<VertexBuffer>& transformBuffer, const std::vector<SelectTransformData>& transformData, uint32_t instanceCount);
		static void RenderTransparentMesh(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, uint32_t submeshIndex, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<VertexBuffer>& transformBuffer, const std::vector<TransformData>& transformData, uint32_t instanceCount);
		static void RenderMeshWithoutMaterial(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Mesh>& mesh, uint32_t submeshIndex, const std::shared_ptr<VertexBuffer>& transformBuffer, const std::vector<TransformData>& transformData, uint32_t instanceCount);
		static void RenderLines(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, std::shared_ptr<VertexBuffer>& linesData, uint32_t drawCount);
		static void RenderLight(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, const LightSelectData& lightData);
		static void RenderLight(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, const glm::vec4& position);
		static void SubmitFullscreenQuad(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Material>& shaderMaterial);
		static void SubmitAndPresent();

		static std::shared_ptr<Image2D> CreateBRDFTexture();
		static std::shared_ptr<Texture2D> GetWhiteTexture(ImageFormat format = ImageFormat::RGBA8_SRGB);
		static std::shared_ptr<TextureCube> GetBlackTextureCube(ImageFormat format);

		static int GetCurrentFrameIndex();
		static int RT_GetCurrentFrameIndex();
		static RendererConfig GetConfig();
		static std::unordered_map<std::string, std::shared_ptr<Shader>>& GetShaderMap();
		static std::shared_ptr<Shader> GetShader(const std::string& name)
		{
			return m_Shaders.contains(name) ? m_Shaders[name] : nullptr;
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
		static void WaitAndExecute();
	private:
		static std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
		static VulkanRenderer* s_Renderer;
		static RendererConfig s_RendererConfig;
	};

}
