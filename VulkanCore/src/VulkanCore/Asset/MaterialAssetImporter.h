#pragma once
#include "VulkanCore/Asset/AssetMetadata.h"
#include "MaterialAsset.h"

namespace VulkanCore {

	class MaterialAssetImporter
	{
	public:
		static std::shared_ptr<MaterialAsset> ImportMaterialAsset(AssetHandle handle, const AssetMetadata& metadata);

		static void SerializeToYAML(const AssetMetadata& metadata, std::shared_ptr<Asset> asset);
		static void Serialize(const AssetMetadata& metadata, MaterialAsset* materialAsset);
		static bool DeserializeFromYAML(const AssetMetadata& metadata, std::shared_ptr<Asset>& asset);
	private:
	};

}
