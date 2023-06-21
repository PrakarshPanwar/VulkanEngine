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

	void MaterialAsset::SaveAsset()
	{
	}

	void MaterialAsset::NewAsset()
	{
	}

}
