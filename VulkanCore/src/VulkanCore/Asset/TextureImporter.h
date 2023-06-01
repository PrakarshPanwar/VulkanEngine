#pragma once
#include "AssetMetadata.h"
#include "VulkanCore/Renderer/Texture.h"

namespace VulkanCore {

	class TextureImporter
	{
	public:
		static std::shared_ptr<Texture2D> ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata);
		static std::shared_ptr<TextureCube> ImportTextureCube(AssetHandle handle, const AssetMetadata& metadata);
	};

}