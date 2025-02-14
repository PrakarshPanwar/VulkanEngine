#include "vulkanpch.h"
#include "MaterialAsset.h"
#include "AssetManager.h"
#include "MaterialAssetImporter.h"

namespace VulkanCore {

	MaterialAsset::MaterialAsset(std::shared_ptr<Material> material)
		: m_Material(material)
	{
	}

	MaterialAsset::~MaterialAsset()
	{
	}

	void MaterialAsset::Serialize()
	{
		AssetMetadata metadata = AssetManager::GetMetadata(Handle);
		MaterialAssetImporter::Serialize(metadata, this);
	}

	void MaterialAsset::SetTransparent(bool transparent)
	{
		m_Transparent = transparent;
	}

	void MaterialAsset::SetDisplacement(bool displacement)
	{
		m_Displacement = displacement;
	}

	MaterialTable::MaterialTable(const std::map<uint32_t, std::shared_ptr<MaterialAsset>>& materials)
		: m_Materials(materials)
	{
	}

	void MaterialTable::SetMaterial(uint32_t index, std::shared_ptr<MaterialAsset> material)
	{
		m_Materials[index] = material;
	}

	std::shared_ptr<MaterialAsset> MaterialTable::GetMaterial(uint32_t index) const
	{
		return m_Materials.at(index);
	}

	bool MaterialTable::HasMaterial(uint32_t index) const
	{
		return (m_Materials.at(index) != nullptr) && m_Materials.contains(index);
	}

}
