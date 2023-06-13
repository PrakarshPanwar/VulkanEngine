#include "vulkanpch.h"
#include "Asset.h"

#include "VulkanCore/Core/Core.h"

namespace VulkanCore {

	namespace Utils {

		std::string AssetTypeToString(AssetType type)
		{
			switch (type)
			{
			case AssetType::None:		 return "None";
			case AssetType::Scene:		 return "Scene";
			case AssetType::Texture2D:	 return "Texture2D";
			case AssetType::TextureCube: return "TextureCube";
			case AssetType::Mesh:		 return "Mesh";
			case AssetType::MeshAsset:	 return "MeshAsset";
			case AssetType::Material:	 return "Material";
			default:
				VK_CORE_ASSERT(false, "Asset Type not found!");
				return {};
			}
		}

		AssetType AssetTypeFromString(const std::string& assetType)
		{
			if (assetType == "None")		return AssetType::None;
			if (assetType == "Scene")		return AssetType::Scene;
			if (assetType == "Texture2D")	return AssetType::Texture2D;
			if (assetType == "TextureCube") return AssetType::TextureCube;
			if (assetType == "Mesh")		return AssetType::Mesh;
			if (assetType == "MeshAsset")	return AssetType::MeshAsset;
			if (assetType == "Material")	return AssetType::Material;

			VK_CORE_ASSERT(false, "Asset Type not found!");
			return AssetType::None;
		}

	}

}
