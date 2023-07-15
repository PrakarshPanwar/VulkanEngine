#include "vulkanpch.h"
#include "Renderer.h"

#include "Platform/Vulkan/VulkanRenderCommandBuffer.h"
#include "Platform/Vulkan/VulkanMaterial.h"
#include "Platform/Vulkan/VulkanRenderer.h"
#include "Platform/Vulkan/VulkanShader.h"

namespace VulkanCore {

	std::unordered_map<std::string, std::shared_ptr<Shader>> Renderer::m_Shaders;
	VulkanRenderer* Renderer::s_Renderer = nullptr;
	RendererConfig Renderer::s_RendererConfig = {};

	namespace Utils {

		std::shared_ptr<Shader> MakeShader(const std::string& path)
		{
			const std::filesystem::path shaderPath = "assets\\shaders";
			std::filesystem::path vertexShaderPath = shaderPath / path, fragmentShaderPath = shaderPath / path, computeShaderPath = shaderPath / path;
			vertexShaderPath.replace_extension(".vert");
			fragmentShaderPath.replace_extension(".frag");
			computeShaderPath.replace_extension(".comp");

			if (std::filesystem::exists(vertexShaderPath) && std::filesystem::exists(fragmentShaderPath))
				return std::make_shared<VulkanShader>(vertexShaderPath.string(), fragmentShaderPath.string());

			if (std::filesystem::exists(computeShaderPath))
				return std::make_shared<VulkanShader>(computeShaderPath.string());

			VK_CORE_ASSERT(false, "Shader: {} does not exist!", path);
			return std::make_shared<VulkanShader>();
		}

	}

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
		m_Shaders["CorePBR"] = Utils::MakeShader("CorePBR");
		m_Shaders["LightShader"] = Utils::MakeShader("LightShader");
		m_Shaders["ExtComposite"] = Utils::MakeShader("ExtComposite");
		m_Shaders["SceneComposite"] = Utils::MakeShader("SceneComposite");
		m_Shaders["Bloom"] = Utils::MakeShader("Bloom");
		m_Shaders["DOF"] = Utils::MakeShader("DOF");
		m_Shaders["Skybox"] = Utils::MakeShader("Skybox");
		m_Shaders["EquirectangularToCubeMap"] = Utils::MakeShader("EquirectangularToCubeMap");
		m_Shaders["EnvironmentMipFilter"] = Utils::MakeShader("EnvironmentMipFilter");
		m_Shaders["EnvironmentIrradiance"] = Utils::MakeShader("EnvironmentIrradiance");
		m_Shaders["GenerateBRDF"] = Utils::MakeShader("GenerateBRDF");
	}

	void Renderer::ShutDown()
	{
		RenderThread::ExecuteDeletionQueue();
		m_Shaders.clear();
	}

	void Renderer::RenderSkybox(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<VertexBuffer>& skyboxVB, const std::shared_ptr<Material>& skyboxMaterial, void* pcData)
	{
		s_Renderer->RenderSkybox(cmdBuffer, pipeline, skyboxVB, skyboxMaterial, pcData);
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

	void Renderer::RenderTransparentMesh(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, uint32_t submeshIndex, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<VertexBuffer>& transformBuffer, const std::vector<TransformData>& transformData, uint32_t instanceCount)
	{
		s_Renderer->RenderTransparentMesh(cmdBuffer, mesh, material, submeshIndex, pipeline, transformBuffer, transformData, instanceCount);
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