#include "vulkanpch.h"
#include "AssetImporter.h"
#include "TextureImporter.h"

#include "VulkanCore/Core/Core.h"

#include <map>

namespace VulkanCore {

	namespace Utils {

		std::string AssetTypeToString(AssetType type)
		{
			switch (type)
			{
			case AssetType::None:		 return "None";
			case AssetType::Scene:		 return "Scene";
			case AssetType::Texture2D:	 return "Texture 2D";
			case AssetType::TextureCube: return "Texture Cube";
			case AssetType::Mesh:		 return "Mesh";
			case AssetType::Material:	 return "Material";
			default:
				VK_CORE_ASSERT(false, "Asset Type not found!");
				return {};
			}
		}

	}

	using AssetImportFunction = std::function<std::shared_ptr<Asset>(AssetHandle, const AssetMetadata&)>;
	static std::map<AssetType, AssetImportFunction> s_AssetImportFunctions = {
		{ AssetType::Texture2D, TextureImporter::ImportTexture2D },
		{ AssetType::TextureCube, TextureImporter::ImportTextureCube }
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

}