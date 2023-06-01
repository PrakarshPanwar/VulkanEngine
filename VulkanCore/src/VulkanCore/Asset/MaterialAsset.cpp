#include "vulkanpch.h"
#include "MaterialAsset.h"
#include "MaterialAssetSerializer.h"

namespace VulkanCore {

	MaterialAsset::MaterialAsset()
	{
	}

	MaterialAsset::~MaterialAsset()
	{
	}

	void MaterialAsset::SaveAsset()
	{
		MaterialAssetSerializer assetSerializer{};
		assetSerializer.SerializeToYAML(*this);
	}

	void MaterialAsset::NewAsset()
	{

	}

}
