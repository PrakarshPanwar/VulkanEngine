#include "vulkanpch.h"
#include "EditorAssetManagerBase.h"
#include "AssetImporter.h"

#include "VulkanCore/Core/Core.h"

#include <yaml-cpp/yaml.h>

namespace VulkanCore {

	bool EditorAssetManagerBase::IsAssetHandleValid(AssetHandle handle) const
	{
		return handle != 0 && m_AssetRegistry.contains(handle);
	}

	bool EditorAssetManagerBase::IsAssetLoaded(AssetHandle handle) const
	{
		return m_LoadedAssets.contains(handle);
	}

	bool EditorAssetManagerBase::LoadRegistryFromFile()
	{
		std::ifstream stream("assets/AssetRegistry.vkr");
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Asset Registry"])
			return false;

		auto assetRegistry = data["Asset Registry"];
		if (assetRegistry)
		{
			for (auto asset : assetRegistry)
			{
				AssetHandle assetHandle = asset["Handle"].as<uint64_t>();
				AssetMetadata assetMetadata{};

				auto metadata = asset["Metadata"];
				if (metadata)
				{
					assetMetadata.FilePath = metadata["Filepath"].as<std::string>();
					assetMetadata.Type = (AssetType)metadata["Type"].as<uint16_t>();
				}

				m_AssetRegistry[assetHandle] = assetMetadata;
			}
		}

		return true;
	}

	void EditorAssetManagerBase::WriteToAssetRegistry(AssetHandle handle, const AssetMetadata& metadata)
	{
		m_AssetRegistry[handle] = metadata;
	}

	void EditorAssetManagerBase::SetLoadedAsset(AssetHandle handle, std::shared_ptr<Asset> asset)
	{
		m_LoadedAssets[handle] = asset;
	}

	const AssetMetadata& EditorAssetManagerBase::GetMetadata(AssetHandle handle) const
	{
		static AssetMetadata s_NullMetadata;
		auto it = m_AssetRegistry.find(handle);
		if (it == m_AssetRegistry.end())
			return s_NullMetadata;

		return it->second;
	}

	std::shared_ptr<Asset> EditorAssetManagerBase::GetAsset(AssetHandle handle) const
	{
		// 1. Check if handle is valid
		if (!IsAssetHandleValid(handle))
			return nullptr;

		// 2. Check if asset needs load (and if so, load)
		std::shared_ptr<Asset> asset = nullptr;
		if (IsAssetLoaded(handle))
		{
			asset = m_LoadedAssets.at(handle);
		}
		else
		{
			// Load asset
			const AssetMetadata& metadata = GetMetadata(handle);
			asset = AssetImporter::ImportAsset(handle, metadata);
			if (!asset)
			{
				// Import failed
				VK_CORE_ERROR("EditorAssetManager::GetAsset - asset import failed!");
			}
		}
		// 3. Return asset
		return asset;
	}

}
