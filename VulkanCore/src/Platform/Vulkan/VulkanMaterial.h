#pragma once
#include "VulkanCore/Core/Shader.h"
#include "Platform/Vulkan/VulkanTexture.h"

namespace VulkanCore {

	class VulkanMaterial
	{
	public:
		VulkanMaterial(std::shared_ptr<Shader> shader, const std::string& debugName);
		~VulkanMaterial();

		void Invalidate();
		void InvalidateDescriptorSets();

		void SetTexture(uint32_t binding, std::shared_ptr<VulkanImage> image);
		void SetTexture(uint32_t binding, std::shared_ptr<VulkanTexture> texture);
	private:
		std::shared_ptr<Shader> m_Shader;
		std::string m_DebugName;

		std::shared_ptr<VulkanTexture> m_WhiteTexture;

		// NOTE: Material Descriptor Set is Set 0, rest are Pending Descriptor Sets
		std::vector<VkDescriptorSet> m_MaterialDescriptorSets;
		std::vector<VulkanDescriptorWriter> m_MaterialDescriptorWriter;
		// Key: Binding ID, Value: 3-sized Descriptor Sets vector
		std::unordered_map<uint32_t, std::vector<VkDescriptorSet>> m_PendingDescriptorSets;
	};

}
