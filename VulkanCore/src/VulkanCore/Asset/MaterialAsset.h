#pragma once
#include "VulkanCore/Renderer/Material.h"
#include "VulkanCore/Asset/Asset.h"

namespace VulkanCore {

	class MaterialAsset : public Asset
	{
	public:
		MaterialAsset(std::shared_ptr<Material> material);
		~MaterialAsset();

		void Serialize();
		void SetTransparent(bool transparent);
		void SetDisplacement(bool displacement);

		inline std::shared_ptr<Material> GetMaterial() const { return m_Material; }
		inline bool IsTransparent() const { return m_Transparent; }
		inline bool HasDisplacementTexture() const { return m_Displacement; }

		inline AssetType GetType() const override { return AssetType::Material; }
		static AssetType GetStaticType() { return AssetType::Material; }
	private:
		std::shared_ptr<Material> m_Material;
		bool m_Transparent = false, m_Displacement = false;
	};

	class MaterialTable
	{
	public:
		MaterialTable() = default;
		MaterialTable(const std::map<uint32_t, std::shared_ptr<MaterialAsset>>& materials);
		~MaterialTable() = default;

		void SetMaterial(uint32_t index, std::shared_ptr<MaterialAsset> material);

		std::shared_ptr<MaterialAsset> GetMaterial(uint32_t index) const;
		inline const std::map<uint32_t, std::shared_ptr<MaterialAsset>>& GetMaterialMap() const { return m_Materials; }
		bool HasMaterial(uint32_t index) const;
	private:
		// Key: Material Index, Value: Material
		std::map<uint32_t, std::shared_ptr<MaterialAsset>> m_Materials;
	};

}
