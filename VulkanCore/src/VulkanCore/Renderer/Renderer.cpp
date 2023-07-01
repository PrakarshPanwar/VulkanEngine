#include "vulkanpch.h"
#include "Renderer.h"

#include "VulkanCore/Core/Core.h"
#include "Platform/Vulkan/VulkanRenderCommandBuffer.h"
#include "Platform/Vulkan/VulkanMaterial.h"

#include <glm/gtc/type_ptr.hpp>

namespace VulkanCore {

	std::unordered_map<std::string, std::shared_ptr<Shader>> Renderer::m_Shaders;
	VulkanRenderer* Renderer::s_Renderer = nullptr;
	RendererConfig Renderer::s_RendererConfig = {};

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

		glm::vec4 GetLabelColor(DebugLabelColor labelColor)
		{
			switch (labelColor)
			{
			case DebugLabelColor::None:	  return { 0.1f, 0.1f, 0.1f, 1.0f };
			case DebugLabelColor::Grey:	  return { 0.66f, 0.66f, 0.66f, 1.0f };
			case DebugLabelColor::Red:	  return { 1.0f, 0.0f, 0.0f, 1.0f };
			case DebugLabelColor::Blue:	  return { 0.0f, 0.58f, 1.0f, 1.0f };
			case DebugLabelColor::Gold:	  return { 0.76f, 0.7f, 0.32f, 1.0f };
			case DebugLabelColor::Orange: return { 1.0f, 0.34f, 0.2f, 1.0f };
			case DebugLabelColor::Pink:	  return { 0.972f, 0.784f, 0.862f, 1.0f };
			case DebugLabelColor::Aqua:	  return { 0.0f, 1.0f, 1.0f, 1.0f };
			case DebugLabelColor::Green:  return { 0.486f, 0.988f, 0.0f, 1.0f };
			default:
				VK_CORE_ASSERT(false, "Label Color is undefined!");
				return { 0.3f, 0.3f, 0.3f, 1.0f };
			}
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

	void Renderer::BeginRenderPass(const std::shared_ptr<VulkanRenderCommandBuffer>& cmdBuffer, std::shared_ptr<VulkanRenderPass> renderPass)
	{
		renderPass->Begin(cmdBuffer);
	}

	void Renderer::EndRenderPass(const std::shared_ptr<VulkanRenderCommandBuffer>& cmdBuffer, std::shared_ptr<VulkanRenderPass> renderPass)
	{
		renderPass->End(cmdBuffer);
	}

	void Renderer::BuildShaders()
	{
		m_Shaders["CorePBR"] = Utils::MakeShader("CorePBR");
		m_Shaders["LightShader"] = Utils::MakeShader("LightShader");
		m_Shaders["SceneComposite"] = Utils::MakeShader("SceneComposite");
		m_Shaders["Bloom"] = Utils::MakeShader("Bloom");
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

	void Renderer::RenderSkybox(const std::shared_ptr<VulkanRenderCommandBuffer>& cmdBuffer, const std::shared_ptr<VulkanPipeline>& pipeline, const std::shared_ptr<VulkanVertexBuffer>& skyboxVB, const std::shared_ptr<VulkanMaterial>& skyboxMaterial, void* pcData)
	{
		Renderer::Submit([cmdBuffer, pipeline, skyboxMaterial, pcData, skyboxVB]
		{
			VK_CORE_PROFILE_FN("Renderer::RenderSkybox");

			VkCommandBuffer vulkanDrawCmd = cmdBuffer->RT_GetActiveCommandBuffer();
			pipeline->Bind(vulkanDrawCmd);

			vkCmdBindDescriptorSets(vulkanDrawCmd,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipeline->GetVulkanPipelineLayout(),
				0, 1, &skyboxMaterial->RT_GetVulkanMaterialDescriptorSet(),
				0, nullptr);

			vkCmdPushConstants(vulkanDrawCmd,
				pipeline->GetVulkanPipelineLayout(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				(uint32_t)sizeof(glm::vec2),
				pcData);

			VkBuffer skyboxBuffer[] = { skyboxVB->GetVulkanBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(vulkanDrawCmd, 0, 1, skyboxBuffer, offsets);
			vkCmdDraw(vulkanDrawCmd, 36, 1, 0, 0);
		});
	}	
	
	void Renderer::BeginTimestampsQuery(const std::shared_ptr<VulkanRenderCommandBuffer>& cmdBuffer)
	{
		Renderer::Submit([cmdBuffer]
		{
			vkCmdWriteTimestamp(cmdBuffer->RT_GetActiveCommandBuffer(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				cmdBuffer->m_TimestampQueryPool, cmdBuffer->m_TimestampsQueryIndex);
		});
	}

	void Renderer::EndTimestampsQuery(const std::shared_ptr<VulkanRenderCommandBuffer>& cmdBuffer)
	{
		Renderer::Submit([cmdBuffer]
		{
			vkCmdWriteTimestamp(cmdBuffer->RT_GetActiveCommandBuffer(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			cmdBuffer->m_TimestampQueryPool, cmdBuffer->m_TimestampsQueryIndex + 1);

			cmdBuffer->m_TimestampsQueryIndex += 2;
			cmdBuffer->m_TimestampsQueryIndex = cmdBuffer->m_TimestampsQueryIndex % cmdBuffer->m_TimestampQueryBufferSize;
		});
	}

	void Renderer::BeginGPUPerfMarker(const std::shared_ptr<VulkanRenderCommandBuffer>& cmdBuffer, const std::string& name, DebugLabelColor labelColor)
	{
		Renderer::Submit([cmdBuffer, name, labelColor]
		{
			VkCommandBuffer vulkanCmdBuffer = cmdBuffer->RT_GetActiveCommandBuffer();

			glm::vec4 color = Utils::GetLabelColor(labelColor);
			VKUtils::SetCommandBufferLabel(vulkanCmdBuffer, name.c_str(), glm::value_ptr(color));
		});
	}

	void Renderer::EndGPUPerfMarker(const std::shared_ptr<VulkanRenderCommandBuffer>& cmdBuffer)
	{
		Renderer::Submit([cmdBuffer]
		{
			VkCommandBuffer vulkanCmdBuffer = cmdBuffer->RT_GetActiveCommandBuffer();
			VKUtils::EndCommandBufferLabel(vulkanCmdBuffer);
		});
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

	void Renderer::SubmitFullscreenQuad(const std::shared_ptr<VulkanRenderCommandBuffer>& cmdBuffer, const std::shared_ptr<VulkanPipeline>& pipeline, const std::shared_ptr<VulkanMaterial>& shaderMaterial)
	{
		Renderer::Submit([cmdBuffer, pipeline, shaderMaterial]
		{
			VK_CORE_PROFILE_FN("Renderer::SubmitFullscreenQuad");

			VkCommandBuffer vulkanDrawCmd = cmdBuffer->RT_GetActiveCommandBuffer();

			pipeline->Bind(vulkanDrawCmd);

			vkCmdBindDescriptorSets(vulkanDrawCmd,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipeline->GetVulkanPipelineLayout(),
				0, 1, &shaderMaterial->RT_GetVulkanMaterialDescriptorSet(),
				0, nullptr);

			vkCmdDraw(vulkanDrawCmd, 3, 1, 0, 0);
		});
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