#include "vulkanpch.h"
#include "AssetImporter.h"
#include "TextureImporter.h"
#include "MeshImporter.h"
#include "MaterialAssetImporter.h"

#include "VulkanCore/Core/Core.h"

#include <map>

namespace VulkanCore {

	using AssetImportFunction = std::function<std::shared_ptr<Asset>(AssetHandle, const AssetMetadata&)>;
	static std::map<AssetType, AssetImportFunction> s_AssetImportFunctions = {
		{ AssetType::Texture2D, TextureImporter::ImportTexture2D },
		{ AssetType::TextureCube, TextureImporter::ImportTextureCube },
		{ AssetType::Mesh, MeshImporter::ImportMesh },
		{ AssetType::MeshAsset, MeshImporter::ImportAssimpMesh },
		{ AssetType::Material, MaterialAssetImporter::ImportMaterialAsset }
	};

	using AssetSerializer = std::function<void(const AssetMetadata&, std::shared_ptr<Asset>)>;
	static std::map<AssetType, AssetSerializer> s_AssetSerializers = {
		{ AssetType::Texture2D, TextureImporter::SerializeTexture2D },
		{ AssetType::TextureCube, TextureImporter::SerializeTextureCube },
		{ AssetType::Mesh, MeshImporter::SerializeMesh },
		{ AssetType::Material, MaterialAssetImporter::SerializeToYAML }
	};

	using AssetDeserializer = std::function<bool(const AssetMetadata&, std::shared_ptr<Asset>&)>;
	static std::map<AssetType, AssetDeserializer> s_AssetDeserializers = {
		{ AssetType::Mesh, MeshImporter::DeserializeMesh },
		{ AssetType::Material, MaterialAssetImporter::DeserializeFromYAML }
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
		if (!s_AssetSerializers.contains(metadata.Type))
		{
			VK_CORE_ERROR("No serializer available for asset type: {}", Utils::AssetTypeToString(metadata.Type));
			return;
		}

		return s_AssetSerializers.at(metadata.Type)(metadata, asset);
	}

	bool AssetImporter::Deserialize(const AssetMetadata& metadata, std::shared_ptr<Asset>& asset)
	{
		if (!s_AssetDeserializers.contains(metadata.Type))
		{
			VK_CORE_ERROR("No deserializer available for asset type: {}", Utils::AssetTypeToString(metadata.Type));
			return false;
		}

		return s_AssetDeserializers.at(metadata.Type)(metadata, asset);
	}

}