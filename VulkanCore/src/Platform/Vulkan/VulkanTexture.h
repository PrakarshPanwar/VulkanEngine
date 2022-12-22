#pragma once
#include "VulkanDevice.h"
#include <stb_image.h>

#include "VulkanImage.h"

namespace VulkanCore {

	struct TextureSpecification
	{
		uint32_t Width = 0;
		uint32_t Height = 0;
		ImageFormat Format = ImageFormat::RGBA8_SRGB;
		TextureWrap SamplerWrap = TextureWrap::Clamp;
		bool GenerateMips = true;
	};

	class VulkanTexture
	{
	public:
		VulkanTexture() = default;
		VulkanTexture(const std::string& filepath, TextureSpecification spec = {});
		VulkanTexture(uint32_t width, uint32_t height, ImageFormat format);
		~VulkanTexture();

		inline const VulkanImageInfo& GetVulkanImageInfo() const { return m_Info; }
		inline const VkDescriptorImageInfo& GetDescriptorImageInfo() const { return m_Image->GetDescriptorInfo(); }
	private:
		void Invalidate();
		void Release();
		void GenerateMipMaps();
	private:
		std::string m_FilePath;

		TextureSpecification m_Specification;

		std::shared_ptr<VulkanImage> m_Image;
		VulkanImageInfo m_Info;
	};

	class VulkanTextureCube
	{
	public:
		VulkanTextureCube(const std::string& filepath, TextureSpecification spec = {});
		VulkanTextureCube(uint32_t width, uint32_t height, ImageFormat format = ImageFormat::RGBA32F);

		~VulkanTextureCube();

		void Invalidate();
		void GenerateMipMaps(bool readonly);
		VkImageView CreateImageViewSingleMip(uint32_t mip);
		inline const VkDescriptorImageInfo& GetDescriptorImageInfo() const { return m_DescriptorImageInfo; }
	private:
		void Release();
	private:
		std::string m_FilePath;
		std::vector<VkImageView> m_MipReferences;

		TextureSpecification m_Specification;
		VulkanImageInfo m_Info;
		VkDescriptorImageInfo m_DescriptorImageInfo;
	};

}