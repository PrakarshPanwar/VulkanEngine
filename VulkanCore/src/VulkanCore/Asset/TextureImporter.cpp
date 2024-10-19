#include "vulkanpch.h"
#include "TextureImporter.h"
#include "AssetManager.h"

#include <stb_image.h>
#include <algorithm>

namespace VulkanCore {

	std::shared_ptr<Texture2D> TextureImporter::ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata)
	{
		return LoadTexture2D(metadata.FilePath.string());
	}

	// NOTE: This can only be utilized when we have our own cubemap format
	std::shared_ptr<TextureCube> TextureImporter::ImportTextureCube(AssetHandle handle, const AssetMetadata& metadata)
	{
		return nullptr;
	}

	void TextureImporter::SerializeTexture2D(const AssetMetadata& metadata, std::shared_ptr<Asset> asset)
	{
		std::shared_ptr<Texture2D> texture = std::dynamic_pointer_cast<Texture2D>(asset);
	}

	void TextureImporter::SerializeTextureCube(const AssetMetadata& metadata, std::shared_ptr<Asset> asset)
	{

	}

	// TODO: For channels we have to create runtime Texture(similar to Mesh)
	// that will store channel format and Texture Path or maybe we have to switch our swapchain to UNORM format
	std::shared_ptr<Texture2D> TextureImporter::LoadTexture2D(const std::string& path)
	{
		int width = 0, height = 0, channels = 0;
		void* data = nullptr;

		TextureSpecification spec{};
		if (stbi_is_hdr(path.c_str()))
		{
			spec.Format = ImageFormat::RGBA32F;
			data = (uint8_t*)stbi_loadf(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		}
		else if (!path.empty())
		{
			std::string pathStr{};
			pathStr.resize(path.size());

			std::transform(path.begin(), path.end(), pathStr.begin(), [](char c) { return std::tolower(c); });

			if (pathStr.find("nor") != std::string::npos || pathStr.find("arm") != std::string::npos || pathStr.find("disp") != std::string::npos)
				spec.Format = ImageFormat::RGBA8_UNORM;

			data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		}

		spec.Width = width;
		spec.Height = height;

		std::shared_ptr<Texture2D> texture = Texture2D::Create(data, spec);
		return texture;
	}

}
