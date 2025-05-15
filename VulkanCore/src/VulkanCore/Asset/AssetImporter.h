#pragma once
#include "AssetMetadata.h"

namespace VulkanCore {

	class AssetImporter
	{
	public:
		static std::shared_ptr<Asset> ImportAsset(AssetHandle handle, const AssetMetadata& metadata);
		static void Serialize(const AssetMetadata& metadata, std::shared_ptr<Asset> asset);
		static bool Deserialize(const AssetMetadata& metadata, std::shared_ptr<Asset>& asset);
	};

}
