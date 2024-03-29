#pragma once
#include "AssetManagerBase.h"
#include "AssetMetadata.h"

namespace VulkanCore {

	using AssetRegistry = std::unordered_map<AssetHandle, AssetMetadata>;

	class EditorAssetManager : public AssetManagerBase
	{
	public:
		std::shared_ptr<Asset> GetAsset(AssetHandle handle) override;

		bool IsAssetHandleValid(AssetHandle handle) const override;
		bool IsAssetLoaded(AssetHandle handle) const override;

		void WriteToAssetRegistry(AssetHandle handle, const AssetMetadata& metadata);
		void SetLoadedAsset(AssetHandle handle, std::shared_ptr<Asset> asset);

		// const types
		const AssetRegistry& GetAssetRegistry() const { return m_AssetRegistry; }
		const AssetMap& GetAssetMap() const { return m_LoadedAssets; }
		const AssetMap& GetMemoryAssetMap() const { return m_MemoryAssets; }
		const AssetMetadata& GetMetadata(AssetHandle handle) const;

		// non-const types
		AssetRegistry& GetAssetRegistry() { return m_AssetRegistry; }
		AssetMap& GetAssetMap() { return m_LoadedAssets; }
		AssetMap& GetMemoryAssetMap() { return m_MemoryAssets; }

		void ClearLoadedAssets();
		void ClearMemoryOnlyAssets();
		void ClearAssetRegistry();
	private:
		AssetRegistry m_AssetRegistry;
		AssetMap m_LoadedAssets;
		AssetMap m_MemoryAssets;
	};

}
