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
		VulkanTexture(const std::string& filepath);
		VulkanTexture(uint32_t width, uint32_t height);
		~VulkanTexture();

		inline const VulkanImageInfo& GetVulkanImageInfo() const { return m_Info; }
		inline const VkDescriptorImageInfo& GetDescriptorImageInfo() const { return m_Image->GetDescriptorInfo(); }
	private:
		/// Deprecated Methods
		void CreateTextureImage();
		void CreateImage();
		void CreateTextureImageView();
		void CreateTextureSampler();
		void TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
		/// Deprecated Methods
		void Invalidate();
		void Release();
		void GenerateMipMaps();
	private:
		std::string m_FilePath;

		TextureSpecification m_Specification;

		std::shared_ptr<VulkanImage> m_Image;
		VulkanImageInfo m_Info;
		VkDeviceMemory m_TextureImageMemory;

		// TODO: These members will be removed after deprecated methods are removed
		stbi_uc* m_Pixels;
		int m_TexWidth, m_TexHeight, m_Channels; 
	};



}