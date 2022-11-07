#include "vulkanpch.h"
#include "Renderer.h"
#include "VulkanRenderer.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

namespace VulkanCore {

	std::unordered_map<std::string, std::shared_ptr<Shader>> Renderer::m_Shaders;
	std::vector<VkCommandBuffer> Renderer::m_CommandBuffers;

	namespace Utils {

		std::shared_ptr<Shader> MakeShader(const std::string& path)
		{
			std::filesystem::path vertexShaderPath = path, fragmentShaderPath = path;
			vertexShaderPath.replace_extension(".vert");
			fragmentShaderPath.replace_extension(".frag");

			bool shaderassert = std::filesystem::exists(vertexShaderPath) && std::filesystem::exists(fragmentShaderPath);

			VK_CORE_ASSERT(shaderassert, "Shader: {} is Incomplete!", path);

			return std::make_shared<Shader>(vertexShaderPath.string(), fragmentShaderPath.string());
		}

	}
	
	void Renderer::SetCommandBuffers(const std::vector<VkCommandBuffer>& cmdBuffers)
	{
		m_CommandBuffers = cmdBuffers;
	}

	int Renderer::GetCurrentFrameIndex()
	{
		return VulkanRenderer::Get()->GetCurrentFrameIndex();
	}

	void Renderer::BeginRenderPass(std::shared_ptr<VulkanRenderPass> renderPass)
	{
		auto beginPassCmd = m_CommandBuffers[GetCurrentFrameIndex()];
		renderPass->Begin(beginPassCmd);
	}

	void Renderer::EndRenderPass(std::shared_ptr<VulkanRenderPass> renderPass)
	{
		auto endPassCmd = m_CommandBuffers[GetCurrentFrameIndex()];
		renderPass->End(endPassCmd);
	}

	void Renderer::BuildShaders()
	{
		m_Shaders["FirstShader"] = Utils::MakeShader("assets/shaders/FirstShader");
		m_Shaders["PointLightShader"] = Utils::MakeShader("assets/shaders/PointLightShader");
	}

	void Renderer::DestroyShaders()
	{
		m_Shaders.clear();
	}

	void Renderer::WaitandRender()
	{
		RenderThread::WaitandDestroy();
	}

}