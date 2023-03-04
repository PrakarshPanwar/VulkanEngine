#include "vulkanpch.h"
#include "VulkanMaterial.h"

#include "VulkanSwapChain.h"
#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Core/ImGuiLayer.h"
#include "VulkanCore/Scene/SceneRenderer.h"

namespace VulkanCore {

	VulkanMaterial::VulkanMaterial(const std::string& debugName)
		: m_DebugName(debugName)
	{
		Invalidate();
		InvalidateDescriptorSets();
	}

	VulkanMaterial::~VulkanMaterial()
	{
	}

	void VulkanMaterial::Invalidate()
	{
		auto descriptorSetPool = Application::Get()->GetDescriptorPool();

		auto shader = SceneRenderer::GetSceneRenderer()->GetGeometryPipelineShader();
		auto materialSetLayout = shader->GetDescriptorSetLayout(1);
		VkDescriptorSetLayout vulkanMaterialSetLayout = materialSetLayout->GetVulkanDescriptorSetLayout();
		for (uint32_t i = 0; i < VulkanSwapChain::MaxFramesInFlight; ++i)
		{
			auto& vulkanDescriptorSet = m_MaterialDescriptorSets.emplace_back();
			descriptorSetPool->AllocateDescriptorSet(vulkanMaterialSetLayout, vulkanDescriptorSet);
		}

		m_MaterialDescriptorWriter = std::vector<VulkanDescriptorWriter>(3, { *materialSetLayout, *descriptorSetPool });

		InitializeMaterialTextures();
	}

	void VulkanMaterial::InvalidateDescriptorSets()
	{
		for (uint32_t i = 0; i < VulkanSwapChain::MaxFramesInFlight; ++i)
		{
			m_MaterialDescriptorWriter[i].WriteImage(0, &m_DiffuseTexture->GetDescriptorImageInfo());
			m_MaterialDescriptorWriter[i].WriteImage(1, &m_NormalTexture->GetDescriptorImageInfo());
			m_MaterialDescriptorWriter[i].WriteImage(2, &m_ARMTexture->GetDescriptorImageInfo());

			m_MaterialDescriptorWriter[i].Build(m_MaterialDescriptorSets[i]);
		}
	}

	void VulkanMaterial::InitializeMaterialTextures()
	{
		m_DiffuseTexture = Renderer::GetWhiteTexture(ImageFormat::RGBA8_SRGB);
		m_NormalTexture = Renderer::GetWhiteTexture(ImageFormat::RGBA8_UNORM);
		m_ARMTexture = Renderer::GetWhiteTexture(ImageFormat::RGBA8_UNORM);

		// For Materials Panel
		m_DiffuseDstID = ImGuiLayer::AddTexture(*m_DiffuseTexture);
		m_NormalDstID = ImGuiLayer::AddTexture(*m_NormalTexture);
		m_ARMDstID = ImGuiLayer::AddTexture(*m_ARMTexture);
	}

}
