#include "vulkanpch.h"
#include "VulkanMaterial.h"

#include "VulkanSwapChain.h"
#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "VulkanCore/Scene/SceneRenderer.h"

namespace VulkanCore {

	VulkanMaterial::VulkanMaterial(const std::string& debugName)
		: m_DebugName(debugName)
	{
		
	}

	VulkanMaterial::~VulkanMaterial()
	{
	}

	void VulkanMaterial::Invalidate()
	{
		auto descriptorSetPool = Application::Get()->GetDescriptorPool();

		auto shader = SceneRenderer::GetSceneRenderer()->GetGeometryPipelineShader();
		auto materialSetLayout = shader->GetDescriptorSetLayout(0);
		VkDescriptorSetLayout vulkanMaterialSetLayout = materialSetLayout->GetVulkanDescriptorSetLayout();
		for (uint32_t i = 0; i < VulkanSwapChain::MaxFramesInFlight; ++i)
			descriptorSetPool->AllocateDescriptorSet(vulkanMaterialSetLayout, m_MaterialDescriptorSets[i]);

		m_MaterialDescriptorWriter = std::vector<VulkanDescriptorWriter>(
			3,
			{ *materialSetLayout, *descriptorSetPool });

		for (uint32_t i = 0; i < VulkanSwapChain::MaxFramesInFlight; ++i)
		{
			m_MaterialDescriptorWriter[i].WriteImage(0, &m_DiffuseTexture->GetDescriptorImageInfo());
			m_MaterialDescriptorWriter[i].WriteImage(1, &m_NormalTexture->GetDescriptorImageInfo());
			m_MaterialDescriptorWriter[i].WriteImage(2, &m_ARMTexture->GetDescriptorImageInfo());

			m_MaterialDescriptorWriter[i].Build(m_MaterialDescriptorSets[i]);
		}
	}

}
