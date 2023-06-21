#pragma once
#include "EditorAssetManager.h"
#include "AssetImporter.h"

namespace VulkanCore {

	class Texture2D;
	class TextureCube;
	class MeshSource;
	class Mesh;
	class MaterialAsset;

	class AssetManager
	{
	public:
		template<typename T>
		static std::shared_ptr<T> ImportNewAsset(const std::string& filepath)
		{
			static_assert(std::derived_from<T, Asset>, "ImportNewAsset only works for types derived from Asset");

			std::filesystem::path assetPath = filepath;

			// NOTE: Maybe in future we can try assigned handle "std.filesystem.hash_value(assetPath)"
			// But for now we are assigning random generated UUIDs
			AssetMetadata metadata = {};
			AssetHandle handle = {}; // Generate Random Handle

			if (std::filesystem::exists(assetPath))
			{
				metadata.FilePath = assetPath;
				metadata.Type = GetAssetType<T>();
			}

			// Create Asset
			std::shared_ptr<Asset> asset = AssetImporter::ImportAsset(handle, metadata);
			asset->Handle = handle;

			std::shared_ptr<EditorAssetManager> editorAssetManager = GetEditorAssetManager();
			editorAssetManager->WriteToAssetRegistry(handle, metadata);
			editorAssetManager->SetLoadedAsset(handle, asset);

			WriteRegistryToFile();

			return std::static_pointer_cast<T>(asset);
		}

		template<typename T, typename... Args>
		static std::shared_ptr<T> CreateNewAsset(const std::string& filepath, Args&&... args)
		{
			static_assert(std::derived_from<T, Asset>, "CreateNewAsset only works for types derived from Asset");

			std::filesystem::path assetPath = filepath;

			// NOTE: Maybe in future we can try assigned handle "std.filesystem.hash_value(assetPath)"
			// But for now we are assigning random generated UUIDs
			AssetHandle handle = {}; // Generate Random Handle
			AssetMetadata metadata = {};
			metadata.FilePath = assetPath;
			metadata.Type = GetAssetType<T>();

			// Create Asset
			std::shared_ptr<T> asset = std::make_shared<T>(std::forward<Args>(args)...);
			asset->Handle = handle;

			std::shared_ptr<EditorAssetManager> editorAssetManager = GetEditorAssetManager();
			editorAssetManager->WriteToAssetRegistry(handle, metadata);
			editorAssetManager->SetLoadedAsset(handle, asset);

			WriteRegistryToFile();

			AssetImporter::Serialize(metadata, asset);

			return asset;
		}

		template<typename T>
		static std::shared_ptr<T> GetAsset(AssetHandle handle)
		{
			static_assert(std::derived_from<T, Asset>, "GetAsset only works for types derived from Asset");

			std::shared_ptr<Asset> asset = s_AssetManager->GetAsset(handle);
			return std::static_pointer_cast<T>(asset);
		}

		// NOTE: This overload is slowest try to use it as less as possible
		template<typename T>
		static std::shared_ptr<T> GetAsset(const std::string& filepath)
		{
			static_assert(std::derived_from<T, Asset>, "GetAsset only works for types derived from Asset");

			std::filesystem::path assetPath = filepath;

			auto editorAssetManager = GetEditorAssetManager();
			auto& assetRegistry = editorAssetManager->GetAssetRegistry();
			auto& assetMap = editorAssetManager->GetAssetMap();

			for (auto&& [handle, metadata] : assetRegistry)
			{
				if (metadata.FilePath == assetPath)
					return GetAsset<T>(handle);
			}

			return nullptr;
		}

		static std::shared_ptr<AssetManagerBase> GetAssetManager() { return s_AssetManager; }
		static const AssetMetadata& GetAssetMetadata(AssetHandle handle) { return GetEditorAssetManager()->GetMetadata(handle); }
		static std::shared_ptr<EditorAssetManager> GetEditorAssetManager()
		{
			return std::static_pointer_cast<EditorAssetManager>(s_AssetManager);
		}

		static void SetAssetManagerBase(std::shared_ptr<AssetManagerBase> assetManager);
	private:
		static bool LoadRegistryFromFile();
		static void WriteRegistryToFile();

		template<typename T>
		static AssetType GetAssetType() { return AssetType::None; }

		template<>
		static AssetType GetAssetType<Texture2D>() { return AssetType::Texture2D; }

		template<>
		static AssetType GetAssetType<TextureCube>() { return AssetType::TextureCube; }

		template<>
		static AssetType GetAssetType<Mesh>() { return AssetType::Mesh; }

		template<>
		static AssetType GetAssetType<MeshSource>() { return AssetType::MeshAsset; }

		template<>
		static AssetType GetAssetType<MaterialAsset>() { return AssetType::Material; }
	private:
		static std::shared_ptr<AssetManagerBase> s_AssetManager;

		friend class EditorLayer;
	};

}
