#pragma once
#include "VulkanCore/Core/Shader.h"

namespace VulkanCore {

	class VulkanMaterial
	{
	public:
		VulkanMaterial(std::shared_ptr<Shader> shader, const std::string& debugName);
		~VulkanMaterial();

		void Invalidate();
		void InvalidateDescriptorSets();
	private:
		std::shared_ptr<Shader> m_Shader;
		std::string m_DebugName;

		// NOTE: Material Descriptor Set is Set 0, rest are Pending Descriptor Sets
		std::vector<VkDescriptorSet> m_MaterialDescriptorSets;
		std::unordered_map<uint32_t, std::vector<VkWriteDescriptorSet>> m_MaterialWriteDescriptors;
		// Key: Binding ID, Value: 3-sized Descriptor Sets vector
		std::unordered_map<uint32_t, std::vector<VkDescriptorSet>> m_PendingDescriptorSets;
	};

}
