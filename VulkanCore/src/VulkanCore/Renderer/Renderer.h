#pragma once
#include "Platform/Vulkan/VulkanPipeline.h"
#include "VulkanCore/Mesh/Mesh.h"
#include "VulkanCore/Core/Shader.h"
#include "VulkanCore/Renderer/RenderThread.h"

#define USE_RENDER_THREAD 1

namespace VulkanCore {

	class Renderer
	{
	public:
		static void SetCommandBuffers(const std::vector<VkCommandBuffer>& cmdBuffers);
		static int GetCurrentFrameIndex();
		static void BeginRenderPass(std::shared_ptr<VulkanRenderPass> renderPass);
		static void EndRenderPass(std::shared_ptr<VulkanRenderPass> renderPass);
		static void BuildShaders();
		static void DestroyShaders();

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
	};

}