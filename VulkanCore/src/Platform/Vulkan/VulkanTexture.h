#pragma once
#include "VulkanDevice.h"
#include <stb_image.h>

#include "VulkanImage.h"

namespace VulkanCore {

	// TODO: Only Copy the Data from Image File to Vulkan Image,
	// Image Creation, Image View Creation, Image Sampler Creation will be handled by VulkanImage
	class VulkanTexture
	{
	public:
		VulkanTexture() = default;
		VulkanTexture(const std::string& filepath);
		VulkanTexture(VkImage image, VkImageView imageView, bool destroyImg = true);
		VulkanTexture(uint32_t width, uint32_t height);
		~VulkanTexture();

		inline const VulkanImageInfo& GetVulkanImageInfo() const { return m_Info; }

		inline VkDescriptorImageInfo GetDescriptorImageInfo() // TODO: Set layout dynamically
		{
			return VkDescriptorImageInfo{ m_Info.Sampler, m_Info.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		}
	private:
		void CreateTextureImage();
		void CreateImage();
		void CreateTextureImageView();
		void TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
		void CreateTextureSampler();
		void Release();
	private:
		std::string m_FilePath;

		VulkanImageInfo m_Info;
		VkDeviceMemory m_TextureImageMemory;

		stbi_uc* m_Pixels;
		int m_Width, m_Height, m_Channels;

		bool m_Release = true;
	};



}