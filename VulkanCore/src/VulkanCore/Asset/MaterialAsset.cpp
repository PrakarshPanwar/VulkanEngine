#include "vulkanpch.h"
#include "MaterialAsset.h"
#include "MaterialAssetSerializer.h"

namespace VulkanCore {

	MaterialAsset::MaterialAsset(const std::string& filepath)
		: m_FilePath(filepath)
	{
		std::filesystem::path materialAssetPath = filepath;

		if (std::filesystem::exists(materialAssetPath))
			m_MaterialAssetHandle = std::filesystem::hash_value(materialAssetPath);
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
