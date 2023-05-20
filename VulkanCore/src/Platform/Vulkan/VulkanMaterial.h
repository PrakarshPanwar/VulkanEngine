#pragma once
#include "VulkanCore/Core/Shader.h"
#include "VulkanCore/Renderer/Material.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "Platform/Vulkan/VulkanTexture.h"
#include "Platform/Vulkan/VulkanUniformBuffer.h"

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

		inline const VkDescriptorSet& GetVulkanMaterialDescriptorSet() const { return m_MaterialDescriptorSets[Renderer::GetCurrentFrameIndex()]; }
		inline const VkDescriptorSet& RT_GetVulkanMaterialDescriptorSet() const { return m_MaterialDescriptorSets[Renderer::RT_GetCurrentFrameIndex()]; }
		inline std::tuple<VkDescriptorSet, VkDescriptorSet, VkDescriptorSet> GetMaterialTextureIDs() const
		{
			return { m_DiffuseDstID, m_NormalDstID, m_ARMDstID };
		}

		void SetDiffuseTexture(std::shared_ptr<VulkanTexture> texture) override;
		void SetNormalTexture(std::shared_ptr<VulkanTexture> texture) override;
		void SetARMTexture(std::shared_ptr<VulkanTexture> texture) override;

		void SetImage(uint32_t binding, std::shared_ptr<VulkanImage> image);
		void SetImages(uint32_t binding, const std::vector<std::shared_ptr<VulkanImage>>& images);
		void SetTexture(uint32_t binding, std::shared_ptr<VulkanTexture> texture);
		void SetTexture(uint32_t binding, std::shared_ptr<VulkanTextureCube> textureCube);
		void SetTextures(uint32_t binding, const std::vector<std::shared_ptr<VulkanTexture>>& textures);
		void SetBuffer(uint32_t binding, std::shared_ptr<VulkanUniformBuffer> uniformBuffer);
		void SetBuffers(uint32_t binding, const std::vector<std::shared_ptr<VulkanUniformBuffer>>& uniformBuffers);
		void SetBuffers(uint32_t binding, const std::vector<VulkanUniformBuffer>& uniformBuffers);

		void UpdateMaterials(const std::string& albedo, const std::string& normal, const std::string& arm);
	private:
		void UpdateDiffuseMap(std::shared_ptr<VulkanTexture> diffuse);
		void UpdateNormalMap(std::shared_ptr<VulkanTexture> normal);
		void UpdateARMMap(std::shared_ptr<VulkanTexture> arm);
	private:
		std::string m_DebugName;

		std::shared_ptr<Shader> m_Shader;
		// NOTE: Material Descriptor Set is Set 0, rest are Pending Descriptor Sets
		// Or maybe we can use material per set, so if shader has 5 sets then 5 material objects are required
		std::vector<VkDescriptorSet> m_MaterialDescriptorSets;
		
		// Key: Binding ID, Value: 3-sized Descriptor Sets vector
		std::unordered_map<uint32_t, std::vector<VkWriteDescriptorSet>> m_MaterialDescriptorWriter;

		VkDescriptorSet m_DiffuseDstID, m_NormalDstID, m_ARMDstID;
		// TODO: We will implement this in future for now this is enough
		// Key: Binding ID, Value: 3-sized Descriptor Sets vector
	};

}
