#include "vulkanpch.h"
#include "EditorAssetManager.h"
#include "AssetImporter.h"

#include "VulkanCore/Core/Core.h"

namespace VulkanCore {

	bool EditorAssetManager::IsAssetHandleValid(AssetHandle handle) const
	{
		return handle != 0 && m_AssetRegistry.contains(handle);
	}

	bool EditorAssetManager::IsAssetLoaded(AssetHandle handle) const
	{
		return m_LoadedAssets.contains(handle);
	}

	void EditorAssetManager::WriteToAssetRegistry(AssetHandle handle, const AssetMetadata& metadata)
	{
		m_AssetRegistry[handle] = metadata;
	}

	void EditorAssetManager::SetLoadedAsset(AssetHandle handle, std::shared_ptr<Asset> asset)
	{
		m_LoadedAssets[handle] = asset;
	}

	const AssetMetadata& EditorAssetManager::GetMetadata(AssetHandle handle) const
	{
		static AssetMetadata s_NullMetadata;
		auto it = m_AssetRegistry.find(handle);
		if (it == m_AssetRegistry.end())
			return s_NullMetadata;

		return it->second;
	}

	void EditorAssetManager::ClearLoadedAssets()
	{
		m_LoadedAssets.clear();
	}

	void EditorAssetManager::ClearMemoryOnlyAssets()
	{
		m_MemoryAssets.clear();
	}

	void EditorAssetManager::ClearAssetRegistry()
	{
		m_AssetRegistry.clear();
	}

	std::shared_ptr<Asset> EditorAssetManager::GetAsset(AssetHandle handle)
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
			asset->Handle = handle;
			if (!asset)
			{
				// Import failed
				VK_CORE_ERROR("EditorAssetManager::GetAsset - asset import failed!");
			}

			m_LoadedAssets[handle] = asset;
		}
		// 3. Return asset
		return asset;
	}

}
