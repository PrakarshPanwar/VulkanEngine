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

	void VulkanMaterial::UpdateDiffuseMap(std::shared_ptr<VulkanTexture> diffuse)
	{
		auto device = VulkanContext::GetCurrentDevice();

		SetDiffuseTexture(diffuse);
		for (uint32_t i = 0; i < 3; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptor.dstBinding = 0;
			writeDescriptor.pImageInfo = &diffuse->GetDescriptorImageInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			vkUpdateDescriptorSets(device->GetVulkanDevice(), 1, &writeDescriptor, 0, nullptr);
		}

		// Update ImGui Image Panel
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_DiffuseDstID;
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptor.dstBinding = 0;
			writeDescriptor.pImageInfo = &diffuse->GetDescriptorImageInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			vkUpdateDescriptorSets(device->GetVulkanDevice(), 1, &writeDescriptor, 0, nullptr);
		}
	}

	void VulkanMaterial::UpdateNormalMap(std::shared_ptr<VulkanTexture> normal)
	{
		auto device = VulkanContext::GetCurrentDevice();

		SetNormalTexture(normal);
		for (uint32_t i = 0; i < 3; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptor.dstBinding = 1;
			writeDescriptor.pImageInfo = &normal->GetDescriptorImageInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			vkUpdateDescriptorSets(device->GetVulkanDevice(), 1, &writeDescriptor, 0, nullptr);
		}

		// Update ImGui Image Panel
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_NormalDstID;
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptor.dstBinding = 0;
			writeDescriptor.pImageInfo = &normal->GetDescriptorImageInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			vkUpdateDescriptorSets(device->GetVulkanDevice(), 1, &writeDescriptor, 0, nullptr);
		}
	}

	void VulkanMaterial::UpdateARMMap(std::shared_ptr<VulkanTexture> arm)
	{
		auto device = VulkanContext::GetCurrentDevice();

		SetARMTexture(arm);
		for (uint32_t i = 0; i < 3; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptor.dstBinding = 2;
			writeDescriptor.pImageInfo = &arm->GetDescriptorImageInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			vkUpdateDescriptorSets(device->GetVulkanDevice(), 1, &writeDescriptor, 0, nullptr);
		}

		// Update ImGui Image Panel
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_ARMDstID;
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptor.dstBinding = 0;
			writeDescriptor.pImageInfo = &arm->GetDescriptorImageInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			vkUpdateDescriptorSets(device->GetVulkanDevice(), 1, &writeDescriptor, 0, nullptr);
		}
	}

}
