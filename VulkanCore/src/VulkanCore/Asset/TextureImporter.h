#pragma once
#include "AssetMetadata.h"
#include "VulkanCore/Renderer/Texture.h"

namespace VulkanCore {

	class TextureImporter
	{
	public:
		// Importers
		static std::shared_ptr<Texture2D> ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata);
		static std::shared_ptr<TextureCube> ImportTextureCube(AssetHandle handle, const AssetMetadata& metadata);

		// Serializers
		static void SerializeTexture2D(const AssetMetadata& metadata, std::shared_ptr<Asset> asset);
		static void SerializeTextureCube(const AssetMetadata& metadata, std::shared_ptr<Asset> asset);

		// Loader
		static std::shared_ptr<Texture2D> LoadTexture2D(const std::string& path);
	};

}
