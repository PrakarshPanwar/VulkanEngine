#include "vulkanpch.h"
#include "Texture.h"

#include "Platform/Vulkan/VulkanTexture.h"

namespace VulkanCore {

	std::shared_ptr<Texture2D> Texture2D::Create(void* data, TextureSpecification spec)
	{
		return std::make_shared<VulkanTexture>(data, spec);
	}

	std::shared_ptr<TextureCube> TextureCube::Create(void* data, TextureSpecification spec)
	{
		return std::make_shared<VulkanTextureCube>(data, spec);
	}

}
