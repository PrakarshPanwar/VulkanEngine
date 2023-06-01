#include "vulkanpch.h"
#include "EditorAssetManagerBase.h"
#include "AssetImporter.h"

#include "VulkanCore/Core/Core.h"

namespace VulkanCore {

	bool EditorAssetManagerBase::IsAssetHandleValid(AssetHandle handle) const
	{
		return handle != 0 && m_AssetRegistry.contains(handle);
	}

	bool EditorAssetManagerBase::IsAssetLoaded(AssetHandle handle) const
	{
		return m_LoadedAssets.contains(handle);
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
