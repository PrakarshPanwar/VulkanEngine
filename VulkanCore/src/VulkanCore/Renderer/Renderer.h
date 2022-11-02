#pragma once
#include "Platform/Vulkan/VulkanRenderPass.h"
#include "VulkanCore/Core/Shader.h"

namespace VulkanCore {

	class Renderer
	{
	public:
		static void BeginRenderPass(VkCommandBuffer beginCmd, std::shared_ptr<VulkanRenderPass> renderPass);
		static void EndRenderPass(VkCommandBuffer beginCmd, std::shared_ptr<VulkanRenderPass> renderPass);
		static void BuildShaders();
		static void DestroyShaders();

		static std::shared_ptr<Shader> GetShader(const std::string& name)
		{
			if (!m_Shaders.contains(name))
				return nullptr;

			return m_Shaders[name];
		}
	private:
		static VkCommandBuffer m_CommandBuffer;
		static std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
	};

}