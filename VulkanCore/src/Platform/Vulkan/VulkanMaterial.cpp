#include "vulkanpch.h"
#include "VulkanMaterial.h"
#include "VulkanSwapChain.h"
#include "VulkanCore/Core/Application.h"

namespace VulkanCore {

	VulkanMaterial::VulkanMaterial(std::shared_ptr<Shader> shader, const std::string& debugName)
	{
		auto vulkanDstPool = Application::Get()->GetDescriptorPool();

		VkDescriptorSetLayout materialSetLayout = shader->CreateDescriptorSetLayout(0)->GetDescriptorSetLayout();
		for (uint32_t i = 0; i < VulkanSwapChain::MaxFramesInFlight; ++i)
			vulkanDstPool->AllocateDescriptorSet(materialSetLayout, m_MaterialDescriptorSets[i]);


	}

	VulkanMaterial::~VulkanMaterial()
	{

	}

	void VulkanMaterial::Invalidate()
	{

	}

	void VulkanMaterial::InvalidateDescriptorSets()
	{

	}

}
