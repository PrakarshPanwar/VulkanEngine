#pragma once
#include "VulkanRenderer.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "VulkanCore/Mesh/Mesh.h"
#include "VulkanCore/Core/Shader.h"
#include "VulkanCore/Renderer/RenderThread.h"

#define USE_RENDER_THREAD 0

namespace VulkanCore {

	class VulkanRenderer;

	class Renderer
	{
	public:
		static void SetCommandBuffers(const std::vector<VkCommandBuffer>& cmdBuffers);
		static void SetRendererAPI(VulkanRenderer* vkRenderer);
		static int GetCurrentFrameIndex();
		static void BeginRenderPass(std::shared_ptr<VulkanRenderPass> renderPass);
		static void EndRenderPass(std::shared_ptr<VulkanRenderPass> renderPass);
		static void BuildShaders();
		static void DestroyShaders();

		static void BeginGPUPerfMarker();
		static void EndGPUPerfMarker();
		static void RetrieveQueryPoolResults();
		static uint64_t GetQueryTime(uint32_t index);

		static void SubmitFullscreenQuad(const std::shared_ptr<VulkanPipeline>& pipeline, const std::vector<VkDescriptorSet>& descriptorSet);
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
		static std::vector<VkCommandBuffer> m_CommandBuffers;
		static std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
		static VulkanRenderer* s_Renderer;
		static uint32_t m_QueryIndex;
		static std::array<uint64_t, 10> m_QueryResultBuffer;
	};

}