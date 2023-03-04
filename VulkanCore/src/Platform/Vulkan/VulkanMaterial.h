#pragma once
#include "VulkanCore/Renderer/Material.h"
#include "VulkanCore/Core/Shader.h"
#include "Platform/Vulkan/VulkanTexture.h"

namespace VulkanCore {

	class VulkanMaterial : public Material
	{
	public:
		VulkanMaterial(const std::string& debugName);
		~VulkanMaterial();

		void Invalidate();
		void InvalidateDescriptorSets();

		inline VkDescriptorSet GetVulkanMaterialDescriptorSet(uint32_t index) const { return m_MaterialDescriptorSets[index]; }
	private:
		std::string m_DebugName;

		// NOTE: Material Descriptor Set is Set 0, rest are Pending Descriptor Sets
		std::vector<VkDescriptorSet> m_MaterialDescriptorSets;
		std::vector<VulkanDescriptorWriter> m_MaterialDescriptorWriter;
		// TODO: We will implement this in future for now it is enough
		// Key: Binding ID, Value: 3-sized Descriptor Sets vector
		//std::unordered_map<uint32_t, std::vector<VkDescriptorSet>> m_PendingDescriptorSets;
	};

}
