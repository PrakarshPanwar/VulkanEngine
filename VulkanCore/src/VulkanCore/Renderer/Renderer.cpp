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
			const std::filesystem::path shaderPath = "assets/shaders";
			std::filesystem::path vertexShaderPath = shaderPath / path, fragmentShaderPath = shaderPath / path, computeShaderPath = shaderPath / path;
			vertexShaderPath.replace_extension(".vert");
			fragmentShaderPath.replace_extension(".frag");
			computeShaderPath.replace_extension(".comp");

			if (std::filesystem::exists(vertexShaderPath) && std::filesystem::exists(fragmentShaderPath))
				return std::make_shared<Shader>(vertexShaderPath.string(), fragmentShaderPath.string());

			if (std::filesystem::exists(computeShaderPath))
				return std::make_shared<Shader>(computeShaderPath.string());

			VK_CORE_ASSERT(false, "Shader: {} does not exist!", path);
			return std::make_shared<Shader>();
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
		m_Shaders["CoreShader"] = Utils::MakeShader("CoreShader");
		m_Shaders["PointLight"] = Utils::MakeShader("PointLight");
		m_Shaders["SceneComposite"] = Utils::MakeShader("SceneComposite");
		m_Shaders["Bloom"] = Utils::MakeShader("Bloom");
		m_Shaders["Skybox"] = Utils::MakeShader("Skybox");
		m_Shaders["EquirectangularToCubeMap"] = Utils::MakeShader("EquirectangularToCubeMap");
	}

	void Renderer::DestroyShaders()
	{
		m_Shaders.clear();
	}

	void Renderer::RenderSkybox(const std::shared_ptr<VulkanPipeline>& pipeline, const std::shared_ptr<Mesh>& mesh, const std::vector<VkDescriptorSet>& descriptorSet)
	{
		auto drawCmd = m_CommandBuffers[GetCurrentFrameIndex()];
		auto dstSet = descriptorSet[GetCurrentFrameIndex()];

		pipeline->Bind(drawCmd);

		vkCmdBindDescriptorSets(drawCmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline->GetVulkanPipelineLayout(),
			0, 1, &dstSet,
			0, nullptr);

		// Cube/Spherical Mesh
		mesh->Bind(drawCmd);
		mesh->Draw(drawCmd);
	}

	void Renderer::SubmitFullscreenQuad(const std::shared_ptr<VulkanPipeline>& pipeline, const std::vector<VkDescriptorSet>& descriptorSet)
	{
		auto drawCmd = m_CommandBuffers[GetCurrentFrameIndex()];
		auto dstSet = descriptorSet[GetCurrentFrameIndex()];

		pipeline->Bind(drawCmd);

		vkCmdBindDescriptorSets(drawCmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline->GetVulkanPipelineLayout(),
			0, 1, &dstSet,
			0, nullptr);

		vkCmdDraw(drawCmd, 3, 1, 0, 0);
	}

	void Renderer::RenderMesh(std::shared_ptr<Mesh> mesh)
	{
		mesh->Bind(m_CommandBuffers[GetCurrentFrameIndex()]);
		mesh->Draw(m_CommandBuffers[GetCurrentFrameIndex()]);
	}

	void Renderer::WaitandRender()
	{
		RenderThread::WaitandDestroy();
	}

}