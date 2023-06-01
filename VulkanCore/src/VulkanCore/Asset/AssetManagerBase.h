#pragma once
#include "Asset.h"

namespace VulkanCore {

	using AssetMap = std::unordered_map<AssetHandle, std::shared_ptr<Asset>>;

	class AssetManagerBase
	{
	public:
		virtual std::shared_ptr<Asset> GetAsset(AssetHandle handle) const = 0;

		virtual bool IsAssetHandleValid(AssetHandle handle) const = 0;
		virtual bool IsAssetLoaded(AssetHandle handle) const = 0;
	};

}
