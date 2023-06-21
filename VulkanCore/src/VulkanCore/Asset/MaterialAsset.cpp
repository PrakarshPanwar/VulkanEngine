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
		AssetMetadata metadata = AssetManager::GetAssetMetadata(Handle);
		MaterialAssetImporter::Serialize(metadata, this);
	}

}
