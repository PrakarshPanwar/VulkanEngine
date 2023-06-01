#pragma once
#include "AssetManagerBase.h"
#include "AssetMetadata.h"

namespace VulkanCore {

	using AssetRegistry = std::unordered_map<AssetHandle, AssetMetadata>;

	class EditorAssetManagerBase : public AssetManagerBase
	{
	public:
		std::shared_ptr<Asset> GetAsset(AssetHandle handle) const override;

		bool IsAssetHandleValid(AssetHandle handle) const override;
		bool IsAssetLoaded(AssetHandle handle) const override;

		void WriteToAssetRegistry(AssetHandle handle, const AssetMetadata& metadata);
		void SetLoadedAsset(AssetHandle handle, std::shared_ptr<Asset> asset);

		const AssetRegistry& GetAssetRegistry() const { return m_AssetRegistry; }
		const AssetMap& GetAssetMap() const { return m_LoadedAssets; }
		const AssetMetadata& GetMetadata(AssetHandle handle) const;
	private:
		AssetRegistry m_AssetRegistry;
		AssetMap m_LoadedAssets;

		// TODO: Memory-only assets
	};

}
