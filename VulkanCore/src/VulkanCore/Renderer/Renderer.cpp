#include "vulkanpch.h"
#include "Renderer.h"
#include "VulkanRenderer.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

namespace VulkanCore {

	std::unordered_map<std::string, std::shared_ptr<Shader>> Renderer::m_Shaders;
	std::vector<VkCommandBuffer> Renderer::m_CommandBuffers;
	std::unique_ptr<VulkanBuffer> Renderer::m_QuadBuffer;

	namespace Utils {

		std::shared_ptr<Shader> MakeShader(const std::string& path)
		{
			const std::filesystem::path shaderPath = "assets/shaders";
			std::filesystem::path vertexShaderPath = shaderPath / path, fragmentShaderPath = shaderPath / path;
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
		m_Shaders["CoreShader"] = Utils::MakeShader("CoreShader");
		m_Shaders["PointLight"] = Utils::MakeShader("PointLight");
		m_Shaders["SceneComposite"] = Utils::MakeShader("SceneComposite");
	}

	void Renderer::DestroyShaders()
	{
		m_Shaders.clear();
	}

	void Renderer::CreateQuadBuffer()
	{
		std::vector<QuadVertex> quadVertices = {
			{ glm::vec3{ -1.0f, -1.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f } },
			{ glm::vec3{ -1.0f,  1.0f, 0.0f }, glm::vec2{ 0.0f, 1.0f } },
			{ glm::vec3{  1.0f,  1.0f, 0.0f }, glm::vec2{ 1.0f, 1.0f } },
			{ glm::vec3{  1.0f, -1.0f, 0.0f }, glm::vec2{ 1.0f, 0.0f } }
		};

		auto vertexCount = (uint32_t)quadVertices.size();
		VK_CORE_ASSERT(vertexCount >= 3, "Vertex Count should be at least greater than or equal to 3!");

		VkDeviceSize bufferSize = sizeof(quadVertices[0]) * vertexCount;
		uint32_t vertexSize = sizeof(quadVertices[0]);

		VulkanBuffer stagingBuffer{ vertexSize, vertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

		stagingBuffer.Map();
		stagingBuffer.WriteToBuffer((void*)quadVertices.data());

		m_QuadBuffer = std::make_unique<VulkanBuffer>(vertexSize, vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		auto device = VulkanContext::GetCurrentDevice();

		device->CopyBuffer(stagingBuffer.GetBuffer(), m_QuadBuffer->GetBuffer(), bufferSize);
	}

	void Renderer::ClearResources()
	{
		m_QuadBuffer.reset();
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

		VkBuffer buffers[] = { m_QuadBuffer->GetBuffer() };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(drawCmd, 0, 1, buffers, offsets);
		vkCmdDraw(drawCmd, 4, 1, 0, 0);
	}

	void Renderer::RenderMesh(std::shared_ptr<VulkanMesh> mesh)
	{
		mesh->Bind(m_CommandBuffers[GetCurrentFrameIndex()]);
		mesh->Draw(m_CommandBuffers[GetCurrentFrameIndex()]);
	}

	void Renderer::WaitandRender()
	{
		RenderThread::WaitandDestroy();
	}

}