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
		VulkanMaterial(std::shared_ptr<Shader> shader, const std::string& debugName);
		~VulkanMaterial();

		void InvalidateMaterial();
		void InvalidateMaterialDescriptorSets();
		void Invalidate();
		void InvalidateDescriptorSets();
		void InitializeMaterialTextures();

		inline VkDescriptorSet GetVulkanMaterialDescriptorSet() const { return m_MaterialDescriptorSets[Renderer::GetCurrentFrameIndex()]; }
		inline VkDescriptorSet RT_GetVulkanMaterialDescriptorSet() const { return m_MaterialDescriptorSets[Renderer::RT_GetCurrentFrameIndex()]; }
		inline std::tuple<VkDescriptorSet, VkDescriptorSet, VkDescriptorSet> GetMaterialTextureIDs() const
		{
			return { m_DiffuseDstID, m_NormalDstID, m_ARMDstID };
		}

		//void SetImageInput(uint32_t binding, std::vector<std::shared_ptr<VulkanImage>> images);

		void SetDiffuseTexture(std::shared_ptr<VulkanTexture> texture) override;
		void SetNormalTexture(std::shared_ptr<VulkanTexture> texture) override;
		void SetARMTexture(std::shared_ptr<VulkanTexture> texture) override;

		void UpdateMaterials(const std::string& albedo, const std::string& normal, const std::string& arm);
	private:
		void UpdateDiffuseMap(std::shared_ptr<VulkanTexture> diffuse);
		void UpdateNormalMap(std::shared_ptr<VulkanTexture> normal);
		void UpdateARMMap(std::shared_ptr<VulkanTexture> arm);
	private:
		std::string m_DebugName;

		std::shared_ptr<Shader> m_Shader;
		// NOTE: Material Descriptor Set is Set 0, rest are Pending Descriptor Sets
		std::vector<VkDescriptorSet> m_MaterialDescriptorSets;
		std::vector<VulkanDescriptorWriter> m_MaterialDescriptorWriter;

		VkDescriptorSet m_DiffuseDstID, m_NormalDstID, m_ARMDstID;
		// TODO: We will implement this in future for now this is enough
		// Key: Binding ID, Value: 3-sized Descriptor Sets vector
		std::unordered_map<uint32_t, std::vector<VkDescriptorSet>> m_PendingDescriptorSets;
	};

}
