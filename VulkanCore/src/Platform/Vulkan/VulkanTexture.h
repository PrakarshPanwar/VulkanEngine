#pragma once
#include "VulkanDevice.h"
#include <stb_image.h>

namespace VulkanCore {

	class VulkanTexture
	{
	public:
		VulkanTexture() = default;
		VulkanTexture(const std::string& filepath);
		VulkanTexture(VkImage image, VkImageView imageView);
		~VulkanTexture();

		inline VkImage GetVulkanImage() { return m_TextureImage; }
		inline VkImageView GetVulkanImageView() { return m_TextureImageView; }
		inline VkSampler GetTextureSampler() { return m_TextureSampler; }

		inline VkDescriptorImageInfo GetDescriptorImageInfo()
		{
			return VkDescriptorImageInfo{ m_TextureSampler, m_TextureImageView, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL };
		}

		static uint32_t GetTextureCount() { return m_TextureCount; }
		static std::vector<VkDescriptorImageInfo> GetDescriptorImagesInfo() { return m_DescriptorImagesInfo; }
	private:
		void CreateTextureImage();
		void CreateImage();
		void CreateTextureImageView();
		void TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
		void CreateTextureSampler();
	private:
		std::string m_FilePath;

		VkImage m_TextureImage;
		VkImageView m_TextureImageView;
		VkSampler m_TextureSampler;
		VkDeviceMemory m_TextureImageMemory;
		VmaAllocation m_TextureImageAlloc;

		stbi_uc* m_Pixels;
		int m_Width, m_Height, m_Channels;

		static uint32_t m_TextureCount;
		static std::vector<VkDescriptorImageInfo> m_DescriptorImagesInfo;
	};

}