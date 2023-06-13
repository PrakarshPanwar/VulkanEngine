#include "vulkanpch.h"
#include "AssetImporter.h"
#include "TextureImporter.h"
#include "MeshImporter.h"

#include "VulkanCore/Core/Core.h"

#include <map>

namespace VulkanCore {

	using AssetImportFunction = std::function<std::shared_ptr<Asset>(AssetHandle, const AssetMetadata&)>;
	static std::map<AssetType, AssetImportFunction> s_AssetImportFunctions = {
		{ AssetType::Texture2D, TextureImporter::ImportTexture2D },
		{ AssetType::TextureCube, TextureImporter::ImportTextureCube },
		{ AssetType::Mesh, MeshImporter::ImportMesh },
		{ AssetType::MeshAsset, MeshImporter::ImportAssimpMesh }
	};

	using AssetSerializeFunction = std::function<void(const AssetMetadata&, std::shared_ptr<Asset>)>;
	static std::map<AssetType, AssetSerializeFunction> s_AssetSerializeFunctions = {
		{ AssetType::Texture2D, TextureImporter::SerializeTexture2D },
		{ AssetType::TextureCube, TextureImporter::SerializeTextureCube }
	};

	std::shared_ptr<Asset> AssetImporter::ImportAsset(AssetHandle handle, const AssetMetadata& metadata)
	{
		if (!s_AssetImportFunctions.contains(metadata.Type))
		{
			VK_CORE_ERROR("No importer available for asset type: {}", Utils::AssetTypeToString(metadata.Type));
			return nullptr;
		}

		return s_AssetImportFunctions.at(metadata.Type)(handle, metadata);
	}

	void AssetImporter::Serialize(const AssetMetadata& metadata, std::shared_ptr<Asset> asset)
	{
		return s_AssetSerializeFunctions.at(metadata.Type)(metadata, asset);
	}

}