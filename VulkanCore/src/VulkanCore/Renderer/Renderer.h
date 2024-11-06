#pragma once
#include "VulkanCore/Mesh/Mesh.h"
#include "VulkanCore/Core/Components.h"
#include "VulkanCore/Renderer/Shader.h"
#include "VulkanCore/Renderer/ShaderBindingTable.h"
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
		static bool RayTracing;
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
		static void RenderSelectedMesh(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, uint32_t submeshIndex, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<VertexBuffer>& transformBuffer, const std::vector<SelectTransformData>& transformData, uint32_t instanceCount);
		static void RenderTransparentMesh(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, uint32_t submeshIndex, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<VertexBuffer>& transformBuffer, const std::vector<TransformData>& transformData, uint32_t instanceCount);
		static void RenderLight(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, const LightSelectData& lightData);
		static void RenderLight(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, const glm::vec4& position);
		static void SubmitFullscreenQuad(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Material>& shaderMaterial, void* pcData = nullptr, uint32_t pcSize = 0);
		static void TraceRays(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<RayTracingPipeline>& pipeline, const std::shared_ptr<ShaderBindingTable>& shaderBindingTable, const std::vector<std::shared_ptr<Material>>& shaderMaterials, uint32_t width, uint32_t height, void* pcData, uint32_t pcSize);

		static std::tuple<std::shared_ptr<TextureCube>, std::shared_ptr<TextureCube>> CreateEnviromentMap(const std::shared_ptr<Texture2D>& envTexture);
		static std::tuple<std::shared_ptr<Texture2D>, std::shared_ptr<Texture2D>> CreateCDFPDFTextures(const std::shared_ptr<Texture2D>& hdrTexture);
		static std::shared_ptr<Image2D> CreateBRDFTexture();
		static std::shared_ptr<Texture2D> GetWhiteTexture(ImageFormat format = ImageFormat::RGBA8_SRGB);
		static std::shared_ptr<TextureCube> GetBlackTextureCube(ImageFormat format);
		static void SetRayTracing(bool rayTracing) { s_RendererConfig.RayTracing = rayTracing; }

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
		static void WaitAndRender();
		static void WaitAndExecute();
	private:
		static std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
		static VulkanRenderer* s_Renderer;
		static RendererConfig s_RendererConfig;
	};

}