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
	private:
		static VkCommandBuffer m_CommandBuffer;
		static std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
	};

}