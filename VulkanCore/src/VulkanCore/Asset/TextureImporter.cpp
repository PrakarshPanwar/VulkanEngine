#include "vulkanpch.h"
#include "TextureImporter.h"
#include "AssetManager.h"

#include <stb_image.h>

namespace VulkanCore {

	std::shared_ptr<Texture2D> TextureImporter::ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata)
	{
		if (!AssetManager::GetAssetManager()->IsAssetHandleValid(handle))
			return nullptr;

		int width = 0, height = 0, channels = 0;
		void* data = nullptr;

		TextureSpecification spec{};
		std::string pathStr = metadata.FilePath.string();
		if (stbi_is_hdr(pathStr.c_str()))
		{
			spec.Format = ImageFormat::RGBA32F;
			data = (uint8_t*)stbi_loadf(pathStr.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		}

		else if (!pathStr.empty())
			data = stbi_load(pathStr.c_str(), &width, &height, &channels, STBI_rgb_alpha);

		spec.Width = width;
		spec.Height = height;

		std::shared_ptr<Texture2D> texture = Texture2D::Create(data, spec);
		return texture;
	}

	// NOTE: This can only be utilized when we have our own cubemap format
	std::shared_ptr<TextureCube> TextureImporter::ImportTextureCube(AssetHandle handle, const AssetMetadata& metadata)
	{
		return nullptr;
	}

}
