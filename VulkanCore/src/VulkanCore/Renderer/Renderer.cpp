#include "vulkanpch.h"
#include "Renderer.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

#include <filesystem>

namespace VulkanCore {

	std::unordered_map<std::string, std::shared_ptr<Shader>> Renderer::m_Shaders;

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

	void Renderer::BeginRenderPass(VkCommandBuffer beginPassCmd, std::shared_ptr<VulkanRenderPass> renderPass)
	{
		renderPass->Begin(beginPassCmd);
	}

	void Renderer::EndRenderPass(VkCommandBuffer endPassCmd, std::shared_ptr<VulkanRenderPass> renderPass)
	{
		renderPass->End(endPassCmd);
	}

	void Renderer::BuildShaders()
	{
		m_Shaders["FirstShader"] = Utils::MakeShader("assets/shaders/FirstShader");
		m_Shaders["PointLightShader"] = Utils::MakeShader("assets/shaders/PointLightShader");
	}

}