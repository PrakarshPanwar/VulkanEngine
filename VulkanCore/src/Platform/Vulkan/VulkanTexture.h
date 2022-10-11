#pragma once
#include "VulkanDevice.h"
#include <stb_image.h>

#include "VulkanImage.h"

namespace VulkanCore {

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
	private:
		std::string m_FilePath;

		std::shared_ptr<VulkanImage> m_Image;
		VulkanImageInfo m_Info;
		VkDeviceMemory m_TextureImageMemory;

		stbi_uc* m_Pixels;
		int m_Width, m_Height, m_Channels;
	};



}