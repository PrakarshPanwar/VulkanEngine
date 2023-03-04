#pragma once
#include <glm/glm.hpp>
#include "Platform/Vulkan/VulkanTexture.h"

namespace VulkanCore {

	struct MaterialData
	{
		glm::vec4 Albedo{ 1.0f };
		float Roughness = 1.0f;
		float Metallic = 1.0f;
		uint32_t UseNormalMap = 0;
	};

	class Scene;

	class Material
	{
	public:
		Material() = default;
		Material(const MaterialData& materialData);

		void SetAlbedo(const glm::vec4& albedo);
		void SetMetallic(float mettalic);
		void SetRoughness(float roughness);
		void SetMaterialData(MaterialData materialData);

		void SetDiffuseTexture(std::shared_ptr<VulkanTexture> texture);
		void SetNormalTexture(std::shared_ptr<VulkanTexture> texture);
		void SetARMTexture(std::shared_ptr<VulkanTexture> texture);

		inline MaterialData& GetMaterialData() { return m_MaterialData; }
	protected:
		MaterialData m_MaterialData{};

		std::shared_ptr<VulkanTexture> m_DiffuseTexture;
		std::shared_ptr<VulkanTexture> m_NormalTexture;
		std::shared_ptr<VulkanTexture> m_ARMTexture;
	};

}
