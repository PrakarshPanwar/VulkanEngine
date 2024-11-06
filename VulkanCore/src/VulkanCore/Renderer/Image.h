#pragma once
#include "Resource.h"
#include <glm/glm.hpp>

namespace VulkanCore {

	enum class ImageFormat
	{
		None,
		R8_UNORM,
		R32I,
		R32F,
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
		ReadAttachment,
		Texture
	};

	enum class DependencyType
	{
		None = 0,
		GraphicsToShaderRead,
		GraphicsToComputeRead
	};

	struct ImageSpecification
	{
		std::string DebugName;

		uint32_t Width, Height;
		uint32_t Samples = 1;
		uint32_t MipLevels = 1;
		uint32_t Layers = 1;
		ImageFormat Format;
		ImageUsage Usage;
		bool Transfer = false;
		TextureWrap SamplerWrap = TextureWrap::Clamp;
	};

	class Image2D : public Resource
	{
	public:
		virtual void Resize(uint32_t width, uint32_t height, uint32_t mips = 1) = 0;
		virtual glm::uvec2 GetMipSize(uint32_t mipLevel) const = 0;
		virtual const ImageSpecification& GetSpecification() const = 0;
	};

}
