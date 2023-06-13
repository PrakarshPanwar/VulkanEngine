#pragma once
#include <glm/glm.hpp>
#include "VulkanCore/Renderer/Texture.h"

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

		std::tuple<AssetHandle, AssetHandle, AssetHandle> GetMaterialHandles() const;

		virtual void SetDiffuseTexture(std::shared_ptr<Texture2D> texture);
		virtual void SetNormalTexture(std::shared_ptr<Texture2D> texture);
		virtual void SetARMTexture(std::shared_ptr<Texture2D> texture);

		inline MaterialData& GetMaterialData() { return m_MaterialData; }
	protected:
		MaterialData m_MaterialData{};

		std::shared_ptr<Texture2D> m_DiffuseTexture;
		std::shared_ptr<Texture2D> m_NormalTexture;
		std::shared_ptr<Texture2D> m_ARMTexture;
	};

}
