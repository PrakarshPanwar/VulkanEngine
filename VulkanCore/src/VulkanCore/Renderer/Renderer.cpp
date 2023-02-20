#include "vulkanpch.h"
#include "Renderer.h"

#include "VulkanCore/Core/Core.h"
#include "Platform/Vulkan/VulkanRenderCommandBuffer.h"

namespace VulkanCore {

	std::unordered_map<std::string, std::shared_ptr<Shader>> Renderer::m_Shaders;
	VulkanRenderer* Renderer::s_Renderer = nullptr;

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

	void Renderer::SetRendererAPI(VulkanRenderer* vkRenderer)
	{
		s_Renderer = vkRenderer;
	}

	int Renderer::GetCurrentFrameIndex()
	{
		return VulkanRenderer::Get()->GetCurrentFrameIndex();
	}

	void Renderer::BeginRenderPass(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, std::shared_ptr<VulkanRenderPass> renderPass)
	{
		auto beginPassCmd = cmdBuffer->GetActiveCommandBuffer();
		renderPass->Begin(beginPassCmd);
	}

	void Renderer::EndRenderPass(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, std::shared_ptr<VulkanRenderPass> renderPass)
	{
		auto endPassCmd = cmdBuffer->GetActiveCommandBuffer();
		renderPass->End(endPassCmd);
	}

	void Renderer::BuildShaders()
	{
		m_Shaders["CorePBR"] = Utils::MakeShader("CorePBR");
		m_Shaders["PointLight"] = Utils::MakeShader("PointLight");
		m_Shaders["SceneComposite"] = Utils::MakeShader("SceneComposite");
		m_Shaders["Bloom"] = Utils::MakeShader("Bloom");
		m_Shaders["Skybox"] = Utils::MakeShader("Skybox");
		m_Shaders["EquirectangularToCubeMap"] = Utils::MakeShader("EquirectangularToCubeMap");
		m_Shaders["EnvironmentMipFilter"] = Utils::MakeShader("EnvironmentMipFilter");
		m_Shaders["EnvironmentIrradiance"] = Utils::MakeShader("EnvironmentIrradiance");
		m_Shaders["GenerateBRDF"] = Utils::MakeShader("GenerateBRDF");
	}

	void Renderer::DestroyShaders()
	{
		m_Shaders.clear();
	}

	void Renderer::RenderSkybox(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, std::shared_ptr<VulkanPipeline> pipeline, std::shared_ptr<VulkanVertexBuffer> skyboxVB, const std::vector<VkDescriptorSet>& descriptorSet, void* pcData /*= nullptr*/)
	{
		auto drawCmd = cmdBuffer->GetActiveCommandBuffer();
		auto dstSet = descriptorSet[GetCurrentFrameIndex()];

		pipeline->Bind(drawCmd);

		if (pcData)
			pipeline->SetPushConstants(drawCmd, pcData, sizeof(glm::vec2));

		vkCmdBindDescriptorSets(drawCmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline->GetVulkanPipelineLayout(),
			0, 1, &dstSet,
			0, nullptr);

		VkBuffer skyboxBuffer[] = { skyboxVB->GetVulkanBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(drawCmd, 0, 1, skyboxBuffer, offsets);
		vkCmdDraw(drawCmd, 36, 1, 0, 0);
	}	
	
	void Renderer::BeginGPUPerfMarker(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer)
	{
		vkCmdWriteTimestamp(cmdBuffer->GetActiveCommandBuffer(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			cmdBuffer->m_TimestampQueryPool, cmdBuffer->m_TimestampsQueryIndex);
	}

	void Renderer::EndGPUPerfMarker(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer)
	{
		vkCmdWriteTimestamp(cmdBuffer->GetActiveCommandBuffer(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			cmdBuffer->m_TimestampQueryPool, cmdBuffer->m_TimestampsQueryIndex + 1);

		cmdBuffer->m_TimestampsQueryIndex += 2;
		cmdBuffer->m_TimestampsQueryIndex = cmdBuffer->m_TimestampsQueryIndex % cmdBuffer->m_TimestampQueryBufferSize;
	}

	std::shared_ptr<VulkanTexture> Renderer::GetWhiteTexture(ImageFormat format)
	{
		TextureSpecification whiteTexSpec;
		whiteTexSpec.Width = 1;
		whiteTexSpec.Height = 1;
		whiteTexSpec.Format = format;
		whiteTexSpec.GenerateMips = false;

		uint32_t* textureData = new uint32_t;
		*textureData = 0xffffffff;
		auto whiteTexture = std::make_shared<VulkanTexture>(textureData, whiteTexSpec);
		return whiteTexture;
	}

	void Renderer::SubmitFullscreenQuad(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, const std::shared_ptr<VulkanPipeline>& pipeline, const std::vector<VkDescriptorSet>& descriptorSet)
	{
		auto drawCmd = cmdBuffer->GetActiveCommandBuffer();
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
	}

	void Renderer::WaitandRender()
	{
		RenderThread::Wait();
	}

}