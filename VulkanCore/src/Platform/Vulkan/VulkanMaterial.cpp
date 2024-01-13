#include "vulkanpch.h"
#include "VulkanMaterial.h"

#include "VulkanCore/Core/ImGuiLayer.h"
#include "VulkanCore/Scene/SceneRenderer.h"
#include "VulkanRenderer.h"
#include "VulkanShader.h"
#include "VulkanUniformBuffer.h"
#include "VulkanStorageBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanComputePipeline.h"

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

		auto shader = std::dynamic_pointer_cast<VulkanShader>(m_Shader);
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
		SetTexture(0, std::static_pointer_cast<VulkanTexture>(m_DiffuseTexture));
		SetTexture(1, std::static_pointer_cast<VulkanTexture>(m_NormalTexture));
		SetTexture(2, std::static_pointer_cast<VulkanTexture>(m_ARMTexture));

		InvalidateDescriptorSets();
	}

	void VulkanMaterial::Invalidate()
	{
		auto descriptorSetPool = VulkanRenderer::Get()->GetDescriptorPool();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		auto shader = std::dynamic_pointer_cast<VulkanShader>(m_Shader);
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
		m_DiffuseDstID = ImGuiLayer::AddTexture(*std::static_pointer_cast<VulkanTexture>(m_DiffuseTexture));
		m_NormalDstID = ImGuiLayer::AddTexture(*std::static_pointer_cast<VulkanTexture>(m_NormalTexture));
		m_ARMDstID = ImGuiLayer::AddTexture(*std::static_pointer_cast<VulkanTexture>(m_ARMTexture));
	}

	void VulkanMaterial::SetDiffuseTexture(std::shared_ptr<Texture2D> texture)
	{
		m_DiffuseTexture = texture;
		UpdateDiffuseMap(std::static_pointer_cast<VulkanTexture>(m_DiffuseTexture));
	}

	void VulkanMaterial::SetNormalTexture(std::shared_ptr<Texture2D> texture)
	{
		m_NormalTexture = texture;
		UpdateNormalMap(std::static_pointer_cast<VulkanTexture>(m_NormalTexture));
	}

	void VulkanMaterial::SetARMTexture(std::shared_ptr<Texture2D> texture)
	{
		m_ARMTexture = texture;
		UpdateARMMap(std::static_pointer_cast<VulkanTexture>(m_ARMTexture));
	}

	void VulkanMaterial::SetImage(uint32_t binding, std::shared_ptr<Image2D> image)
	{
		auto shader = std::dynamic_pointer_cast<VulkanShader>(m_Shader);
		auto vulkanImage = std::dynamic_pointer_cast<VulkanImage>(image);

		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = shader->GetDescriptorSetLayout(0)->GetVulkanDescriptorType(binding);
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pImageInfo = &vulkanImage->GetDescriptorImageInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}

	void VulkanMaterial::SetImage(uint32_t binding, std::shared_ptr<Image2D> image, uint32_t mipLevel)
	{
		auto shader = std::dynamic_pointer_cast<VulkanShader>(m_Shader);
		auto vulkanImage = std::dynamic_pointer_cast<VulkanImage>(image);

		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = shader->GetDescriptorSetLayout(0)->GetVulkanDescriptorType(binding);
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pImageInfo = &vulkanImage->GetDescriptorImageInfo(mipLevel);
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}

	void VulkanMaterial::SetTexture(uint32_t binding, std::shared_ptr<Texture2D> texture)
	{
		auto vulkanTexture = std::static_pointer_cast<VulkanTexture>(texture);

		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pImageInfo = &vulkanTexture->GetDescriptorImageInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}


	void VulkanMaterial::SetTexture(uint32_t binding, std::shared_ptr<TextureCube> textureCube)
	{
		auto vulkanTextureCube = std::static_pointer_cast<VulkanTextureCube>(textureCube);

		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pImageInfo = &vulkanTextureCube->GetDescriptorImageInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}

	void VulkanMaterial::SetBuffer(uint32_t binding, std::shared_ptr<UniformBuffer> uniformBuffer)
	{
		auto vulkanUB = std::static_pointer_cast<VulkanUniformBuffer>(uniformBuffer);

		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pBufferInfo = &vulkanUB->GetDescriptorBufferInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}

	void VulkanMaterial::SetBuffer(uint32_t binding, std::shared_ptr<StorageBuffer> storageBuffer)
	{
		auto vulkanSB = std::static_pointer_cast<VulkanStorageBuffer>(storageBuffer);

		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pBufferInfo = &vulkanSB->GetDescriptorBufferInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}

	void VulkanMaterial::SetImages(uint32_t binding, const std::vector<std::shared_ptr<Image2D>>& images)
	{
		auto shader = std::dynamic_pointer_cast<VulkanShader>(m_Shader);

		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			auto vulkanImage = std::static_pointer_cast<VulkanImage>(images[i]);

			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = shader->GetDescriptorSetLayout(0)->GetVulkanDescriptorType(binding);
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pImageInfo = &vulkanImage->GetDescriptorImageInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}

	void VulkanMaterial::SetTextures(uint32_t binding, const std::vector<std::shared_ptr<Texture2D>>& textures)
	{
		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			auto vulkanTexture = std::static_pointer_cast<VulkanTexture>(textures[i]);

			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pImageInfo = &vulkanTexture->GetDescriptorImageInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}

	void VulkanMaterial::SetBuffers(uint32_t binding, const std::vector<std::shared_ptr<UniformBuffer>>& uniformBuffers)
	{
		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			auto vulkanUB = std::static_pointer_cast<VulkanUniformBuffer>(uniformBuffers[i]);

			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pBufferInfo = &vulkanUB->GetDescriptorBufferInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}

	void VulkanMaterial::SetBuffers(uint32_t binding, const std::vector<std::shared_ptr<StorageBuffer>>& storageBuffers)
	{
		std::vector<VkWriteDescriptorSet> writeDescriptors{};
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			auto vulkanSB = std::static_pointer_cast<VulkanStorageBuffer>(storageBuffers[i]);

			VkWriteDescriptorSet writeDescriptor{};
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.dstSet = m_MaterialDescriptorSets[i];
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescriptor.dstBinding = binding;
			writeDescriptor.pBufferInfo = &vulkanSB->GetDescriptorBufferInfo();
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.dstArrayElement = 0;

			writeDescriptors.push_back(writeDescriptor);
		}

		m_MaterialDescriptorWriter[binding] = writeDescriptors;
	}

	void VulkanMaterial::RT_BindMaterial(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, uint32_t setIndex)
	{
		auto vulkanPipeline = std::static_pointer_cast<VulkanPipeline>(pipeline);
		auto vulkanRenderCB = std::static_pointer_cast<VulkanRenderCommandBuffer>(cmdBuffer);

		VkCommandBuffer bindCmd = vulkanRenderCB->RT_GetActiveCommandBuffer();
		VkDescriptorSet descriptorSet[1] = { RT_GetVulkanMaterialDescriptorSet() };

		vkCmdBindDescriptorSets(bindCmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			vulkanPipeline->GetVulkanPipelineLayout(),
			setIndex,
			1,
			descriptorSet,
			0,
			nullptr);
	}

	void VulkanMaterial::RT_BindMaterial(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<ComputePipeline>& pipeline, uint32_t setIndex)
	{
		auto vulkanPipeline = std::static_pointer_cast<VulkanComputePipeline>(pipeline);
		auto vulkanRenderCB = std::static_pointer_cast<VulkanRenderCommandBuffer>(cmdBuffer);

		VkCommandBuffer bindCmd = vulkanRenderCB->RT_GetActiveCommandBuffer();
		VkDescriptorSet descriptorSet[1] = { RT_GetVulkanMaterialDescriptorSet() };

		vkCmdBindDescriptorSets(bindCmd,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			vulkanPipeline->GetVulkanPipelineLayout(),
			setIndex,
			1,
			descriptorSet,
			0,
			nullptr);
	}

	void VulkanMaterial::UpdateMaterials()
	{
		auto device = VulkanContext::GetCurrentDevice();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		std::vector<VkWriteDescriptorSet> writeDescriptors{};

		// Diffuse Texture
		{
			std::shared_ptr<VulkanTexture> diffuseTexture = std::dynamic_pointer_cast<VulkanTexture>(m_DiffuseTexture);

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

		// Normal Texture
 		{
			std::shared_ptr<VulkanTexture> normalTexture = std::dynamic_pointer_cast<VulkanTexture>(m_NormalTexture);

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

		// ARM Texture
		{
			std::shared_ptr<VulkanTexture> armTexture = std::dynamic_pointer_cast<VulkanTexture>(m_ARMTexture);

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
