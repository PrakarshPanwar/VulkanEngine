#include "vulkanpch.h"
#include "VulkanMaterial.h"

#include "VulkanSwapChain.h"
#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Core/ImGuiLayer.h"
#include "VulkanCore/Scene/SceneRenderer.h"

namespace VulkanCore {

	VulkanMaterial::VulkanMaterial(const std::string& debugName)
		: m_DebugName(debugName), m_Shader(Renderer::GetShader("CorePBR"))
	{
		InvalidateMaterial();
		InvalidateMaterialDescriptorSets();
	}

	VulkanMaterial::VulkanMaterial(std::shared_ptr<Shader> shader, const std::string& debugName)
		: m_DebugName(debugName), m_Shader(shader)
	{
		Invalidate();
	}

	VulkanMaterial::~VulkanMaterial()
	{
		// TODO: Another solution to this could be to reset and reallocate whole new pool but it is currently unfeasible
		Renderer::SubmitResourceFree([descriptorSets = m_MaterialDescriptorSets]
		{
			auto device = VulkanContext::GetCurrentDevice();
			auto descriptorPool = VulkanRenderer::Get()->GetDescriptorPool();

			vkFreeDescriptorSets(device->GetVulkanDevice(), descriptorPool->GetVulkanDescriptorPool(), (uint32_t)descriptorSets.size(), descriptorSets.data());
		});

		m_MaterialDescriptorSets.clear();
	}

	void VulkanMaterial::InvalidateMaterial()
	{
		auto descriptorSetPool = VulkanRenderer::Get()->GetDescriptorPool();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		auto shader = m_Shader;
		auto materialSetLayout = shader->GetDescriptorSetLayout(1);
		VkDescriptorSetLayout vulkanMaterialSetLayout = materialSetLayout->GetVulkanDescriptorSetLayout();
		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			auto& vulkanDescriptorSet = m_MaterialDescriptorSets.emplace_back();
			descriptorSetPool->AllocateDescriptorSet(vulkanMaterialSetLayout, vulkanDescriptorSet);
		}

		InitializeMaterialTextures();
	}

	void VulkanMaterial::InvalidateMaterialDescriptorSets()
	{
		SetTexture(0, m_DiffuseTexture);
		SetTexture(1, m_NormalTexture);
		SetTexture(2, m_ARMTexture);

		InvalidateDescriptorSets();
	}

	void VulkanMaterial::Invalidate()
	{
		auto descriptorSetPool = VulkanRenderer::Get()->GetDescriptorPool();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		auto shader = m_Shader;
		auto materialSetLayout = shader->GetDescriptorSetLayout(0);
		VkDescriptorSetLayout vulkanMaterialSetLayout = materialSetLayout->GetVulkanDescriptorSetLayout();
		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			auto& vulkanDescriptorSet = m_MaterialDescriptorSets.emplace_back();
			descriptorSetPool->AllocateDescriptorSet(vulkanMaterialSetLayout, vulkanDescriptorSet);
		}
	}

	void VulkanMaterial::InvalidateDescriptorSets()
	{
		auto device = VulkanContext::GetCurrentDevice();

		std::vector<VkWriteDescriptorSet> finalWriteDescriptorSet{};
		for (auto&& [binding, writeDescriptors] : m_MaterialDescriptorWriter)
			finalWriteDescriptorSet.insert(finalWriteDescriptorSet.end(), writeDescriptors.begin(), writeDescriptors.end());

		vkUpdateDescriptorSets(device->GetVulkanDevice(), (uint32_t)finalWriteDescriptorSet.size(), finalWriteDescriptorSet.data(), 0, nullptr);
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

	void VulkanMaterial::SetImage(uint32_t binding, std::shared_ptr<VulkanImage> image)
	{
		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = m_Shader->GetDescriptorSetLayout(0)->GetVulkanDescriptorType(binding);
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pImageInfo = &image->GetDescriptorImageInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}

	void VulkanMaterial::SetImage(uint32_t binding, std::shared_ptr<VulkanImage> image, uint32_t mipLevel)
	{
		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = m_Shader->GetDescriptorSetLayout(0)->GetVulkanDescriptorType(binding);
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pImageInfo = &image->GetDescriptorImageInfo(mipLevel);
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}

	void VulkanMaterial::SetTexture(uint32_t binding, std::shared_ptr<VulkanTexture> texture)
	{
		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pImageInfo = &texture->GetDescriptorImageInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}


	void VulkanMaterial::SetTexture(uint32_t binding, std::shared_ptr<VulkanTextureCube> textureCube)
	{
		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pImageInfo = &textureCube->GetDescriptorImageInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}

	void VulkanMaterial::SetBuffer(uint32_t binding, std::shared_ptr<VulkanUniformBuffer> uniformBuffer)
	{
		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pBufferInfo = &uniformBuffer->GetDescriptorBufferInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}

	void VulkanMaterial::SetImages(uint32_t binding, const std::vector<std::shared_ptr<VulkanImage>>& images)
	{
		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = m_Shader->GetDescriptorSetLayout(0)->GetVulkanDescriptorType(binding);
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pImageInfo = &images[i]->GetDescriptorImageInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}

	void VulkanMaterial::SetTextures(uint32_t binding, const std::vector<std::shared_ptr<VulkanTexture>>& textures)
	{
		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pImageInfo = &textures[i]->GetDescriptorImageInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}

	void VulkanMaterial::SetBuffers(uint32_t binding, const std::vector<std::shared_ptr<VulkanUniformBuffer>>& uniformBuffers)
	{
		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pBufferInfo = &uniformBuffers[i]->GetDescriptorBufferInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}


	void VulkanMaterial::SetBuffers(uint32_t binding, const std::vector<VulkanUniformBuffer>& uniformBuffers)
	{
		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pBufferInfo = &uniformBuffers[i].GetDescriptorBufferInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}

	void VulkanMaterial::RT_BindMaterial(const std::shared_ptr<VulkanRenderCommandBuffer>& cmdBuffer, const std::shared_ptr<VulkanPipeline>& pipeline, uint32_t setIndex)
	{
		VkCommandBuffer bindCmd = cmdBuffer->RT_GetActiveCommandBuffer();
		VkDescriptorSet descriptorSet[1] = { RT_GetVulkanMaterialDescriptorSet() };

		vkCmdBindDescriptorSets(bindCmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline->GetVulkanPipelineLayout(),
			setIndex,
			1,
			descriptorSet,
			0,
			nullptr);
	}

	void VulkanMaterial::RT_BindMaterial(const std::shared_ptr<VulkanRenderCommandBuffer>& cmdBuffer, const std::shared_ptr<VulkanComputePipeline>& pipeline, uint32_t setIndex /*= 0*/)
	{
		VkCommandBuffer bindCmd = cmdBuffer->RT_GetActiveCommandBuffer();
		VkDescriptorSet descriptorSet[1] = { RT_GetVulkanMaterialDescriptorSet() };

		vkCmdBindDescriptorSets(bindCmd,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			pipeline->GetVulkanPipelineLayout(),
			setIndex,
			1,
			descriptorSet,
			0,
			nullptr);
	}

	void VulkanMaterial::UpdateMaterials(const std::string& albedo, const std::string& normal, const std::string& arm)
	{
		auto device = VulkanContext::GetCurrentDevice();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		if (!albedo.empty())
		{
			std::shared_ptr<VulkanTexture> diffuseTexture = std::make_shared<VulkanTexture>(albedo, ImageFormat::RGBA8_SRGB);
			m_DiffuseTexture = diffuseTexture;

			// Update Material Texture
			for (uint32_t i = 0; i < framesInFlight; ++i)
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
			for (uint32_t i = 0; i < framesInFlight; ++i)
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
			for (uint32_t i = 0; i < framesInFlight; ++i)
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

	void VulkanMaterial::PrepareShaderMaterial()
	{
		InvalidateDescriptorSets();
	}

	void VulkanMaterial::UpdateDiffuseMap(std::shared_ptr<VulkanTexture> diffuse)
	{
		auto device = VulkanContext::GetCurrentDevice();

		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
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
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
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
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
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
