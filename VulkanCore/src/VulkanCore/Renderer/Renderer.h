#pragma once
#include "VulkanRenderer.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "VulkanCore/Mesh/Mesh.h"
#include "VulkanCore/Core/Shader.h"
#include "VulkanCore/Renderer/RenderThread.h"

#define USE_RENDER_THREAD 1

namespace VulkanCore {

	class VulkanRenderer;
	class VulkanRenderCommandBuffer;

	class Renderer
	{
	public:
		static void SetRendererAPI(VulkanRenderer* vkRenderer);
		static int GetCurrentFrameIndex();
		static void BeginRenderPass(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, std::shared_ptr<VulkanRenderPass> renderPass);
		static void EndRenderPass(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, std::shared_ptr<VulkanRenderPass> renderPass);
		static void BuildShaders();
		static void DestroyShaders();

		static void RenderSkybox(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, std::shared_ptr<VulkanPipeline> pipeline, std::shared_ptr<VulkanVertexBuffer> skyboxVB, const std::vector<VkDescriptorSet>& descriptorSet, void* pcData = nullptr);
		static void BeginGPUPerfMarker(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer);
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

		static void WaitandRender();
	private:
		static std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
		static VulkanRenderer* s_Renderer;
	};

}