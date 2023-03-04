#pragma once
#include "VulkanCore/Core/Shader.h"
#include "VulkanCore/Renderer/Material.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "Platform/Vulkan/VulkanTexture.h"

namespace VulkanCore {

	class VulkanMaterial : public Material
	{
	public:
		VulkanMaterial(const std::string& debugName);
		~VulkanMaterial();

		void Invalidate();
		void InvalidateDescriptorSets();
		void InitializeMaterialTextures();

		inline VkDescriptorSet GetVulkanMaterialDescriptorSet() const { return m_MaterialDescriptorSets[Renderer::GetCurrentFrameIndex()]; }
		inline std::tuple<VkDescriptorSet, VkDescriptorSet, VkDescriptorSet> GetMaterialTextureIDs() const
		{
			return { m_DiffuseDstID, m_NormalDstID, m_ARMDstID };
		}
	private:
		std::string m_DebugName;

		// NOTE: Material Descriptor Set is Set 0, rest are Pending Descriptor Sets
		std::vector<VkDescriptorSet> m_MaterialDescriptorSets;
		std::vector<VulkanDescriptorWriter> m_MaterialDescriptorWriter;

		VkDescriptorSet m_DiffuseDstID, m_NormalDstID, m_ARMDstID;
		// TODO: We will implement this in future for now this is enough
		// Key: Binding ID, Value: 3-sized Descriptor Sets vector
		//std::unordered_map<uint32_t, std::vector<VkDescriptorSet>> m_PendingDescriptorSets;
	};

}
