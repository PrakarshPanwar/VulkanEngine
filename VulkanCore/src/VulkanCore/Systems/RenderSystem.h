#pragma once
#include "Platform/Vulkan/VulkanDevice.h"
#include "Platform/Vulkan/VulkanGameObject.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "VulkanCore/Renderer/Camera.h"
#include "VulkanCore/Core/FrameInfo.h"
#include "VulkanCore/Scene/Scene.h"

namespace VulkanCore {

	class RenderSystem
	{
	public:
		RenderSystem(VulkanDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~RenderSystem();

		void RenderGameObjects(FrameInfo& frameInfo);

		inline VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
		inline VulkanPipeline* GetPipeline() const { return m_Pipeline.get(); }
	private:
		void CreatePipeline(VkRenderPass renderPass);
		void CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout);
	private:
		VulkanDevice& m_VulkanDevice;

		std::unique_ptr<VulkanPipeline> m_Pipeline;
		VkPipelineLayout m_PipelineLayout;
	};

}