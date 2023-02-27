#pragma once

#include <glm/glm.hpp>

namespace VulkanCore {

	struct MaterialData
	{
		glm::vec4 Albedo{ 1.0f };
		float Roughness = 1.0f;
		float Metallic = 1.0f;
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

		inline MaterialData& GetMaterialData() { return m_MaterialData; }
	private:
		MaterialData m_MaterialData{};
	};

}
