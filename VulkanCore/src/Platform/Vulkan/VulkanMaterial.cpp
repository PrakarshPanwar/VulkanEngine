#include "vulkanpch.h"
#include "VulkanMaterial.h"

#include "VulkanSwapChain.h"
#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Renderer/Renderer.h"

namespace VulkanCore {

	VulkanMaterial::VulkanMaterial(std::shared_ptr<Shader> shader, const std::string& debugName)
		: m_Shader(shader), m_DebugName(debugName), m_WhiteTexture(Renderer::GetWhiteTexture())
	{
		auto vulkanDstPool = Application::Get()->GetDescriptorPool();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		auto vulkanSetLayout = shader->CreateDescriptorSetLayout(0);
		VkDescriptorSetLayout materialSetLayout = vulkanSetLayout->GetDescriptorSetLayout();
		for (uint32_t i = 0; i < framesInFlight; ++i)
			vulkanDstPool->AllocateDescriptorSet(materialSetLayout, m_MaterialDescriptorSets[i]);

		m_MaterialDescriptorWriter = std::vector<VulkanDescriptorWriter>(
			3,
			{ *vulkanSetLayout, *vulkanDstPool });

		// Only Texture Bindings
		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			for (auto&& [binding, setLayout] : vulkanSetLayout->m_Bindings)
			{
				if (setLayout.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
					setLayout.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				{
					if (setLayout.descriptorCount > 1)
					{
						std::vector<VkDescriptorImageInfo> imageDescriptorInfos(setLayout.descriptorCount, m_WhiteTexture->GetDescriptorImageInfo());
						m_MaterialDescriptorWriter[i].WriteImage(binding, imageDescriptorInfos);
					}

					else
						m_MaterialDescriptorWriter[i].WriteImage(binding, &m_WhiteTexture->GetDescriptorImageInfo());
				}
			}
		}
	}

	VulkanMaterial::~VulkanMaterial()
	{

	}

	void VulkanMaterial::Invalidate()
	{

	}

	void VulkanMaterial::InvalidateDescriptorSets()
	{
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
			m_MaterialDescriptorWriter[i].Build(m_MaterialDescriptorSets[i]);
	}

	void VulkanMaterial::SetTexture(uint32_t binding, std::shared_ptr<VulkanImage> image)
	{
		VkWriteDescriptorSet writeDescriptor;
		writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		//writeDescriptor.descriptorType = m_MaterialDescriptorWriter[i].m_Writes.;
		//writeDescriptor.
	}

}
