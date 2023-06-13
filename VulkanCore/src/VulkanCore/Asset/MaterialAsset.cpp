#include "vulkanpch.h"
#include "MaterialAsset.h"
#include "MaterialAssetImporter.h"

namespace VulkanCore {

	MaterialAsset::MaterialAsset()
	{
	}

	MaterialAsset::~MaterialAsset()
	{
	}

	void MaterialAsset::SaveAsset()
	{
		MaterialAssetImporter assetSerializer{};
		assetSerializer.SerializeToYAML(*this);
	}

	void MaterialAsset::NewAsset()
	{

	}

}
