#pragma once
#include "Resource.h"

namespace VulkanCore {

	enum class ImageFormat
	{
		None,
		RGBA8_SRGB,
		RGBA8_NORM,
		RGBA8_UNORM,
		RGBA16_NORM,
		RGBA16_UNORM,
		RGBA16F,
		RGBA32F,
		R11G11B10F,

		DEPTH24STENCIL8,
		DEPTH16F,
		DEPTH32F
	};

	enum class TextureWrap
	{
		Repeat,
		Clamp
	};

	enum class ImageUsage
	{
		Storage,
		Attachment,
		Texture
	};

	struct ImageSpecification
	{
		std::string DebugName;

		uint32_t Width, Height;
		uint32_t Samples = 1;
		uint32_t MipLevels = 1;
		ImageFormat Format;
		ImageUsage Usage;
		bool Transfer = false;
		TextureWrap SamplerWrap = TextureWrap::Clamp;
	};

	class Image : public Resource
	{

	};

}
