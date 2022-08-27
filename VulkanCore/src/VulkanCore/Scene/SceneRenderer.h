#pragma once
#include "VulkanCore/Core/VulkanDevice.h"
#include "VulkanCore/Core/VulkanPipeline.h"

namespace VulkanCore {

	class SceneRenderer
	{
	public:
		SceneRenderer();
		~SceneRenderer();
	private:
		std::unique_ptr<VulkanPipeline> m_Pipeline;
		VkPipelineLayout m_PipelineLayout;
	};

}