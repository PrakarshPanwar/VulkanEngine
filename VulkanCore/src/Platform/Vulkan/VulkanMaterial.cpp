#include "vulkanpch.h"
#include "VulkanMaterial.h"

#include "VulkanSwapChain.h"
#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Core/ImGuiLayer.h"
#include "VulkanCore/Scene/SceneRenderer.h"

namespace VulkanCore {

	VulkanMaterial::VulkanMaterial(const std::string& debugName)
		: m_DebugName(debugName), m_Shader(SceneRenderer::GetSceneRenderer()->GetGeometryPipelineShader())
	{
		InvalidateMaterial();
		InvalidateMaterialDescriptorSets();
	}

	VulkanMaterial::VulkanMaterial(std::shared_ptr<Shader> shader, const std::string& debugName)
		: m_DebugName(debugName), m_Shader(shader)
	{
	}

	VulkanMaterial::~VulkanMaterial()
	{
	}

	void VulkanMaterial::InvalidateMaterial()
	{
		auto descriptorSetPool = Application::Get()->GetDescriptorPool();

		auto shader = m_Shader;
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

	void VulkanMaterial::InvalidateMaterialDescriptorSets()
	{
		for (uint32_t i = 0; i < VulkanSwapChain::MaxFramesInFlight; ++i)
		{
			m_MaterialDescriptorWriter[i].WriteImage(0, &m_DiffuseTexture->GetDescriptorImageInfo());
			m_MaterialDescriptorWriter[i].WriteImage(1, &m_NormalTexture->GetDescriptorImageInfo());
			m_MaterialDescriptorWriter[i].WriteImage(2, &m_ARMTexture->GetDescriptorImageInfo());

			m_MaterialDescriptorWriter[i].Build(m_MaterialDescriptorSets[i]);
		}
	}

	void VulkanMaterial::Invalidate()
	{
		auto shader = m_Shader;
		auto pipelineSetLayouts = shader->CreateAllDescriptorSetsLayout();

		/*for (auto pipelineSetLayout : pipelineSetLayouts)
		{
			pipelineSetLayout->
		}*/
	}

	void VulkanMaterial::InvalidateDescriptorSets()
	{

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

	void VulkanMaterial::SetDiffuseTexture(std::shared_ptr<VulkanTexture> texture)
	{
		m_DiffuseTexture = texture;
		UpdateDiffuseMap(m_DiffuseTexture);
	}

	void VulkanMaterial::SetNormalTexture(std::shared_ptr<VulkanTexture> texture)
	{
		m_NormalTexture = texture;
		UpdateNormalMap(m_NormalTexture);
	}

	void VulkanMaterial::SetARMTexture(std::shared_ptr<VulkanTexture> texture)
	{
		m_ARMTexture = texture;
		UpdateARMMap(m_ARMTexture);
	}

	void VulkanMaterial::UpdateMaterials(const std::string& albedo, const std::string& normal, const std::string& arm)
	{
		auto device = VulkanContext::GetCurrentDevice();

		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		if (!albedo.empty())
		{
			std::shared_ptr<VulkanTexture> diffuseTexture = std::make_shared<VulkanTexture>(albedo, ImageFormat::RGBA8_SRGB);
			m_DiffuseTexture = diffuseTexture;

			// Update Material Texture
			for (uint32_t i = 0; i < 3; ++i)
			{
				VkWriteDescriptorSet writeDescriptor{};
				writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
				writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptor.dstBinding = 0;
				writeDescriptor.pImageInfo = &diffuseTexture->GetDescriptorImageInfo();
				writeDescriptor.descriptorCount = 1;
				writeDescriptor.dstArrayElement = 0;

				writeDescriptors.push_back(writeDescriptor);
			}

			// Update ImGui Image Panel
			{
				VkWriteDescriptorSet writeDescriptor{};
				writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptor.dstSet = m_DiffuseDstID;
				writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptor.dstBinding = 0;
				writeDescriptor.pImageInfo = &diffuseTexture->GetDescriptorImageInfo();
				writeDescriptor.descriptorCount = 1;
				writeDescriptor.dstArrayElement = 0;

				writeDescriptors.push_back(writeDescriptor);
			}
		}

		if (!normal.empty())
		{
			std::shared_ptr<VulkanTexture> normalTexture = std::make_shared<VulkanTexture>(normal, ImageFormat::RGBA8_UNORM);
			m_NormalTexture = normalTexture;

			// Update Material Texture
			for (uint32_t i = 0; i < 3; ++i)
			{
				VkWriteDescriptorSet writeDescriptor{};
				writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
				writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptor.dstBinding = 1;
				writeDescriptor.pImageInfo = &normalTexture->GetDescriptorImageInfo();
				writeDescriptor.descriptorCount = 1;
				writeDescriptor.dstArrayElement = 0;

				writeDescriptors.push_back(writeDescriptor);
			}

			// Update ImGui Image Panel
			{
				VkWriteDescriptorSet writeDescriptor{};
				writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptor.dstSet = m_NormalDstID;
				writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptor.dstBinding = 0;
				writeDescriptor.pImageInfo = &normalTexture->GetDescriptorImageInfo();
				writeDescriptor.descriptorCount = 1;
				writeDescriptor.dstArrayElement = 0;

				writeDescriptors.push_back(writeDescriptor);
			}
		}

		if (!arm.empty())
		{
			std::shared_ptr<VulkanTexture> armTexture = std::make_shared<VulkanTexture>(arm, ImageFormat::RGBA8_UNORM);
			m_ARMTexture = armTexture;

			// Update Material Texture
			for (uint32_t i = 0; i < 3; ++i)
			{
				VkWriteDescriptorSet writeDescriptor{};
				writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
				writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptor.dstBinding = 2;
				writeDescriptor.pImageInfo = &armTexture->GetDescriptorImageInfo();
				writeDescriptor.descriptorCount = 1;
				writeDescriptor.dstArrayElement = 0;

				writeDescriptors.push_back(writeDescriptor);
			}

			// Update ImGui Image Panel
			{
				VkWriteDescriptorSet writeDescriptor{};
				writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptor.dstSet = m_ARMDstID;
				writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptor.dstBinding = 0;
				writeDescriptor.pImageInfo = &armTexture->GetDescriptorImageInfo();
				writeDescriptor.descriptorCount = 1;
				writeDescriptor.dstArrayElement = 0;

				writeDescriptors.push_back(writeDescriptor);
			}
		}

		vkUpdateDescriptorSets(device->GetVulkanDevice(), (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
	}

	void VulkanMaterial::UpdateDiffuseMap(std::shared_ptr<VulkanTexture> diffuse)
	{
		auto device = VulkanContext::GetCurrentDevice();

		std::vector<VkWriteDescriptorSet> writeDescriptors{};
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

			writeDescriptors.push_back(writeDescriptor);
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

			writeDescriptors.push_back(writeDescriptor);
		}

		vkUpdateDescriptorSets(device->GetVulkanDevice(), (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
	}

	void VulkanMaterial::UpdateNormalMap(std::shared_ptr<VulkanTexture> normal)
	{
		auto device = VulkanContext::GetCurrentDevice();

		std::vector<VkWriteDescriptorSet> writeDescriptors{};
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

			writeDescriptors.push_back(writeDescriptor);
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

			writeDescriptors.push_back(writeDescriptor);
		}

		vkUpdateDescriptorSets(device->GetVulkanDevice(), (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
	}

	void VulkanMaterial::UpdateARMMap(std::shared_ptr<VulkanTexture> arm)
	{
		auto device = VulkanContext::GetCurrentDevice();

		std::vector<VkWriteDescriptorSet> writeDescriptors{};
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

			writeDescriptors.push_back(writeDescriptor);
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

			writeDescriptors.push_back(writeDescriptor);
		}

		vkUpdateDescriptorSets(device->GetVulkanDevice(), (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
	}

}
