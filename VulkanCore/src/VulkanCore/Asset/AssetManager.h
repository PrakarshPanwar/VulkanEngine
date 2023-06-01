#pragma once
#include "EditorAssetManagerBase.h"
#include "AssetMetadata.h"

namespace VulkanCore {

	class AssetManager
	{
	public:
		template<typename T, typename... Args>
		static std::shared_ptr<T> CreateNewAsset(const std::string& filepath, Args&&... args)
		{
			static_assert(std::derived_from<T, Asset>, "CreateNewAsset only works for types derived from Asset");

			std::filesystem::path assetPath = filepath;

			AssetMetadata metadata = {};
			AssetHandle handle = {};

			if (std::filesystem::exists(assetPath))
				metadata.FilePath = assetPath;

			// Create Asset
			std::shared_ptr<T> asset = std::make_shared<T>(std::forward<Args>(args)...);
			asset->Handle = handle;
			metadata.Type = asset->GetType();

			std::shared_ptr<EditorAssetManagerBase> editorAssetManager = std::static_pointer_cast<EditorAssetManagerBase>(s_AssetManager);
			editorAssetManager->WriteToAssetRegistry(handle, metadata);
			editorAssetManager->SetLoadedAsset(handle, asset);

			WriteRegistryToFile();

			return asset;
		}

		template<typename T>
		static std::shared_ptr<T> GetAsset(AssetHandle handle)
		{
			std::shared_ptr<Asset> asset = s_AssetManager->GetAsset(handle);
			return std::static_pointer_cast<T>(asset);
		}

		template<typename T>
		static std::shared_ptr<T> GetAsset(const std::string& filepath)
		{
			std::filesystem::path assetPath = filepath;

			auto editorAssetManager = std::static_pointer_cast<EditorAssetManagerBase>(s_AssetManager);
			auto& assetRegistry = editorAssetManager->GetAssetRegistry();
			auto& assetMap = editorAssetManager->GetAssetMap();

			for (auto&& [handle, metadata] : assetRegistry)
			{
				if (metadata.FilePath == assetPath)
					return assetMap[handle];
			}

			return nullptr;
		}

		static std::shared_ptr<AssetManagerBase> GetAssetManager() { return s_AssetManager; }
		static void SetAssetManager(std::shared_ptr<AssetManagerBase> assetManager);
	private:
		static void WriteRegistryToFile();
	private:
		static std::shared_ptr<AssetManagerBase> s_AssetManager;
	};

}
