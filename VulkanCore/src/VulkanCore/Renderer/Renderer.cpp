#include "vulkanpch.h"
#include "Renderer.h"

#include "Platform/Vulkan/VulkanRenderCommandBuffer.h"
#include "Platform/Vulkan/VulkanMaterial.h"
#include "Platform/Vulkan/VulkanRenderer.h"
#include "Platform/Vulkan/VulkanShader.h"
#include "Platform/Vulkan/VulkanSlangShader.h"

#define VK_CREATE_SHADER(name) m_Shaders[name] = std::make_shared<VulkanShader>(name)
#define VK_CREATE_SLANG_SHADER(name) m_Shaders[name] = std::make_shared<VulkanSlangShader>(name) 

namespace VulkanCore {

	std::unordered_map<std::string, std::shared_ptr<Shader>> Renderer::m_Shaders;
	VulkanRenderer* Renderer::s_Renderer = nullptr;
	RendererConfig Renderer::s_RendererConfig = {};

	void Renderer::SetRendererAPI(VulkanRenderer* vkRenderer)
	{
		s_Renderer = vkRenderer;
	}

	int Renderer::GetCurrentFrameIndex()
	{
		return s_Renderer->GetCurrentFrameIndex();
	}

	int Renderer::RT_GetCurrentFrameIndex()
	{
		return RenderThread::GetThreadFrameIndex();
	}

	RendererConfig Renderer::GetConfig()
	{
		return s_RendererConfig;
	}

	std::unordered_map<std::string, std::shared_ptr<Shader>>& Renderer::GetShaderMap()
	{
		return m_Shaders;
	}

	void Renderer::BeginRenderPass(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, std::shared_ptr<RenderPass> renderPass)
	{
		s_Renderer->BeginRenderPass(cmdBuffer, renderPass);
	}

	void Renderer::EndRenderPass(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, std::shared_ptr<RenderPass> renderPass)
	{
		s_Renderer->EndRenderPass(cmdBuffer, renderPass);
	}

	void Renderer::BuildShaders()
	{
		VulkanSlangShader::CreateGlobalSession();

		VK_CREATE_SLANG_SHADER("CorePBR");
		VK_CREATE_SHADER("CorePBR_Tess"); // TODO: Future support required for Vulkan Tessellation in Slang
		VK_CREATE_SLANG_SHADER("Lines");
		VK_CREATE_SHADER("ShadowDepth"); // TODO: Problem in ShaderLayer SPIR-V
		VK_CREATE_SHADER("CoreEditor");
		VK_CREATE_SHADER("LightShader");
		VK_CREATE_SHADER("LightEditor");
		VK_CREATE_SLANG_SHADER("SceneComposite");
		VK_CREATE_SLANG_SHADER("Bloom");
		VK_CREATE_SLANG_SHADER("Skybox");
		VK_CREATE_SHADER("EquirectangularToCubeMap");
		VK_CREATE_SHADER("EnvironmentMipFilter");
		VK_CREATE_SHADER("EnvironmentIrradiance");
		VK_CREATE_SHADER("GenerateBRDF");
		VK_CREATE_SHADER("GTAO");
	}

	void Renderer::ShutDown()
	{
		RenderThread::ExecuteDeletionQueue();
		m_Shaders.clear();
	}

	void Renderer::RenderSkybox(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Material>& skyboxMaterial, void* pcData)
	{
		s_Renderer->RenderSkybox(cmdBuffer, pipeline, skyboxMaterial, pcData);
	}	
	
	void Renderer::BeginTimestampsQuery(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer)
	{
		s_Renderer->BeginTimestampsQuery(cmdBuffer);
	}

	void Renderer::EndTimestampsQuery(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer)
	{
		s_Renderer->EndTimestampsQuery(cmdBuffer);
	}

	void Renderer::BeginGPUPerfMarker(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::string& name, DebugLabelColor labelColor)
	{
		s_Renderer->BeginGPUPerfMarker(cmdBuffer, name, labelColor);
	}

	void Renderer::EndGPUPerfMarker(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer)
	{
		s_Renderer->EndGPUPerfMarker(cmdBuffer);
	}

	void Renderer::BindPipeline(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Material>& material)
	{
		s_Renderer->BindPipeline(cmdBuffer, pipeline, material);
	}

	void Renderer::CopyVulkanImage(const std::shared_ptr<RenderCommandBuffer>& commandBuffer, const std::shared_ptr<Image2D>& sourceImage, const std::shared_ptr<Image2D>& destImage)
	{
		s_Renderer->CopyVulkanImage(commandBuffer, sourceImage, destImage);
	}

	void Renderer::BlitVulkanImage(const std::shared_ptr<RenderCommandBuffer>& commandBuffer, const std::shared_ptr<Image2D>& image)
	{
		s_Renderer->BlitVulkanImage(commandBuffer, image);
	}

	void Renderer::RenderMesh(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, uint32_t submeshIndex, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<VertexBuffer>& transformBuffer, const std::vector<TransformData>& transformData, uint32_t instanceCount)
	{
		s_Renderer->RenderMesh(cmdBuffer, mesh, material, submeshIndex, pipeline, transformBuffer, transformData, instanceCount);
	}

	void Renderer::RenderSelectedMesh(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Mesh>& mesh, uint32_t submeshIndex, const std::shared_ptr<VertexBuffer>& transformBuffer, const std::vector<SelectTransformData>& transformData, uint32_t instanceCount)
	{
		s_Renderer->RenderSelectedMesh(cmdBuffer, mesh, submeshIndex, transformBuffer, transformData, instanceCount);
	}

	void Renderer::RenderTransparentMesh(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, uint32_t submeshIndex, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<VertexBuffer>& transformBuffer, const std::vector<TransformData>& transformData, uint32_t instanceCount)
	{
		s_Renderer->RenderTransparentMesh(cmdBuffer, mesh, material, submeshIndex, pipeline, transformBuffer, transformData, instanceCount);
	}

	void Renderer::RenderMeshWithoutMaterial(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Mesh>& mesh, uint32_t submeshIndex, const std::shared_ptr<VertexBuffer>& transformBuffer, const std::vector<TransformData>& transformData, uint32_t instanceCount)
	{
		s_Renderer->RenderMeshWithoutMaterial(cmdBuffer, mesh, submeshIndex, transformBuffer, transformData, instanceCount);
	}

	void Renderer::RenderLines(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, std::shared_ptr<VertexBuffer>& linesData, uint32_t drawCount)
	{
		s_Renderer->RenderLines(cmdBuffer, linesData, drawCount);
	}

	void Renderer::RenderLight(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, const LightSelectData& lightData)
	{
		s_Renderer->RenderLight(cmdBuffer, pipeline, lightData);
	}

	void Renderer::RenderLight(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, const glm::vec4& position)
	{
		s_Renderer->RenderLight(cmdBuffer, pipeline, position);
	}

	void Renderer::SubmitFullscreenQuad(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Material>& shaderMaterial)
	{
		s_Renderer->SubmitFullscreenQuad(cmdBuffer, pipeline, shaderMaterial);
	}

	std::shared_ptr<Image2D> Renderer::CreateBRDFTexture()
	{
		return s_Renderer->CreateBRDFTexture();
	}

	std::shared_ptr<Texture2D> Renderer::GetWhiteTexture(ImageFormat format)
	{
		return s_Renderer->GetWhiteTexture(format);
	}

	std::shared_ptr<TextureCube> Renderer::GetBlackTextureCube(ImageFormat format)
	{
		return s_Renderer->GetBlackTextureCube(format);
	}

	void Renderer::Init()
	{
		RenderThread::Init();
	}

	void Renderer::WaitAndRender()
	{
		VK_CORE_PROFILE();

		RenderThread::NextFrame();
		RenderThread::WaitAndSet();
	}

	void Renderer::WaitAndExecute()
	{
		RenderThread::SetAtomicFlag(true);
		RenderThread::NotifyThread();
		RenderThread::Wait();
	}

}