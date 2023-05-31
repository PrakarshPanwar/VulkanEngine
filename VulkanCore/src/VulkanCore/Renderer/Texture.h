#pragma once
#include "Platform/Vulkan/VulkanImage.h"
#include "VulkanCore/Asset/Asset.h"

namespace VulkanCore {

	struct TextureSpecification
	{
		uint32_t Width = 0;
		uint32_t Height = 0;
		ImageFormat Format = ImageFormat::RGBA8_SRGB;
		TextureWrap SamplerWrap = TextureWrap::Repeat;
		bool GenerateMips = true;
	};

	class Texture : public Asset
	{
	public:
		virtual ~Texture() = default;

		virtual const TextureSpecification& GetSpecification() const = 0;
		virtual bool IsLoaded() const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		AssetType GetType() const override { return AssetType::Texture2D; }
	};

	class TextureCube : public Texture
	{
	public:
		AssetType GetType() const override { return AssetType::TextureCube; }
	};

}
