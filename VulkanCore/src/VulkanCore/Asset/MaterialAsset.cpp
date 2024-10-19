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

}
