#include "vulkanpch.h"
#include "VulkanTexture.h"
#include "stb_image.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

#include "VulkanBuffer.h"
#include "VulkanDescriptor.h"

namespace VulkanCore {

	uint32_t VulkanTexture::m_TextureCount = 0;
	std::vector<VkDescriptorImageInfo> VulkanTexture::m_DescriptorImagesInfo;

	VulkanTexture::VulkanTexture(const std::string& filepath)
		: m_FilePath(filepath)
	{
		CreateTextureImage();
		CreateTextureImageView();
		CreateTextureSampler();

		m_TextureCount++;
		m_DescriptorImagesInfo.emplace_back(m_TextureSampler, m_TextureImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	VulkanTexture::~VulkanTexture()
	{
		vkDestroySampler(VulkanDevice::GetDevice()->GetVulkanDevice(), m_TextureSampler, nullptr);
		vkDestroyImageView(VulkanDevice::GetDevice()->GetVulkanDevice(), m_TextureImageView, nullptr);
		vkDestroyImage(VulkanDevice::GetDevice()->GetVulkanDevice(), m_TextureImage, nullptr);
		vkFreeMemory(VulkanDevice::GetDevice()->GetVulkanDevice(), m_TextureImageMemory, nullptr);
	}

	void VulkanTexture::CreateTextureImage()
	{
		m_Pixels = stbi_load(m_FilePath.c_str(), &m_Width, &m_Height, &m_Channels, STBI_rgb_alpha);

		VkDeviceSize imageSize = m_Width * m_Height * 4;
		VK_CORE_ASSERT(m_Pixels, "Failed to Load Image {0}", m_FilePath);

		VulkanBuffer stagingBuffer{ *VulkanDevice::GetDevice(), imageSize, 1, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

		stagingBuffer.Map(imageSize);
		stagingBuffer.WriteToBuffer(m_Pixels, imageSize);
		stagingBuffer.Unmap();

		free(m_Pixels);

		CreateImage();
		TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		VulkanDevice::GetDevice()->CopyBufferToImage(stagingBuffer.GetBuffer(), m_TextureImage, m_Width, m_Height, 1);
		TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void VulkanTexture::CreateImage()
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = m_Width;
		imageInfo.extent.height = m_Height;
		imageInfo.extent.depth = 1;

		uint32_t mipLevels = static_cast<uint32_t>(std::_Floor_of_log_2(std::max(m_Width, m_Height))) + 1;
		imageInfo.mipLevels = mipLevels;

		imageInfo.arrayLayers = 1;
		imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VulkanDevice::GetDevice()->CreateImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage, m_TextureImageMemory);
	}

	void VulkanTexture::TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout)
{
		VkCommandBuffer commandBuffer = VulkanDevice::GetDevice()->BeginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_TextureImage;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}

		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}

		else
			VK_CORE_ASSERT(false, "Unsupported Layout Transition!");

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		VulkanDevice::GetDevice()->EndSingleTimeCommands(commandBuffer);
	}

	void VulkanTexture::CreateTextureSampler()
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;

		auto deviceProps = VulkanDevice::GetDevice()->GetPhysicalDeviceProperties();
		samplerInfo.maxAnisotropy = deviceProps.limits.maxSamplerAnisotropy;

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		VK_CHECK_RESULT(vkCreateSampler(VulkanDevice::GetDevice()->GetVulkanDevice(), &samplerInfo, nullptr, &m_TextureSampler), "Failed to Create Texture Sampler!");
	}

	void VulkanTexture::CreateTextureImageView()
	{
		VkImageViewCreateInfo imageViewInfo{};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.image = m_TextureImage;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = 1;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		VK_CHECK_RESULT(vkCreateImageView(VulkanDevice::GetDevice()->GetVulkanDevice(), &imageViewInfo, nullptr, &m_TextureImageView), "Failed to Create Texture Image View!");
	}

}