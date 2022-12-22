#include "vulkanpch.h"
#include "Renderer.h"

#include "VulkanCore/Core/Core.h"

namespace VulkanCore {

	std::unordered_map<std::string, std::shared_ptr<Shader>> Renderer::m_Shaders;
	std::vector<VkCommandBuffer> Renderer::m_CommandBuffers;
	VulkanRenderer* Renderer::s_Renderer = nullptr;
	uint32_t Renderer::m_QueryIndex = 0;
	std::array<uint64_t, 10> Renderer::m_QueryResultBuffer;

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

	void Renderer::SetRendererAPI(VulkanRenderer* vkRenderer)
	{
		s_Renderer = vkRenderer;
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
		m_Shaders["CorePBR"] = Utils::MakeShader("CorePBR");
		m_Shaders["CoreShader"] = Utils::MakeShader("CoreShader");
		m_Shaders["PointLight"] = Utils::MakeShader("PointLight");
		m_Shaders["SceneComposite"] = Utils::MakeShader("SceneComposite");
		m_Shaders["Bloom"] = Utils::MakeShader("Bloom");
		m_Shaders["Skybox"] = Utils::MakeShader("Skybox");
		m_Shaders["EquirectangularToCubeMap"] = Utils::MakeShader("EquirectangularToCubeMap");
		m_Shaders["EnvironmentMipFilter"] = Utils::MakeShader("EnvironmentMipFilter");
		m_Shaders["EnvironmentIrradiance"] = Utils::MakeShader("EnvironmentIrradiance");
	}

	void Renderer::DestroyShaders()
	{
		m_Shaders.clear();
	}

	void Renderer::RenderSkybox(const std::shared_ptr<VulkanPipeline>& pipeline, const std::shared_ptr<Mesh>& mesh, const std::vector<VkDescriptorSet>& descriptorSet, void* pcData)
	{
		auto drawCmd = m_CommandBuffers[GetCurrentFrameIndex()];
		auto dstSet = descriptorSet[GetCurrentFrameIndex()];

		pipeline->Bind(drawCmd);

		if (pcData)
			pipeline->SetPushConstants(drawCmd, pcData, sizeof(glm::vec2));

		vkCmdBindDescriptorSets(drawCmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline->GetVulkanPipelineLayout(),
			0, 1, &dstSet,
			0, nullptr);

		// Cube/Spherical Mesh
		mesh->Bind(drawCmd);
		mesh->Draw(drawCmd);
	}	
	
	void Renderer::BeginGPUPerfMarker()
	{
		auto writeTimestampCmd = m_CommandBuffers[GetCurrentFrameIndex()];
		vkCmdWriteTimestamp(writeTimestampCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, s_Renderer->GetPerfQueryPool(), m_QueryIndex);
	}

	void Renderer::EndGPUPerfMarker()
	{
		auto writeTimestampCmd = m_CommandBuffers[GetCurrentFrameIndex()];
		vkCmdWriteTimestamp(writeTimestampCmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, s_Renderer->GetPerfQueryPool(), m_QueryIndex + 1);

		m_QueryIndex += 2;
		m_QueryIndex = m_QueryIndex % 8;
	}

	void Renderer::RetrieveQueryPoolResults()
	{
		uint32_t queryBufferSize = (uint32_t)m_QueryResultBuffer.size();

		vkGetQueryPoolResults(VulkanContext::GetCurrentDevice()->GetVulkanDevice(),
			s_Renderer->GetPerfQueryPool(),
			0,
			queryBufferSize, sizeof(uint64_t) * queryBufferSize,
			(void*)m_QueryResultBuffer.data(), sizeof(uint64_t),
			VK_QUERY_RESULT_64_BIT);
	}

	uint64_t Renderer::GetQueryTime(uint32_t index)
	{
		uint64_t timeStamp = m_QueryResultBuffer[(index << 1) + 1] - m_QueryResultBuffer[index << 1];
		return timeStamp;
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
		RenderThread::Wait();
	}

}