#include "vulkanpch.h"
#include "VulkanTexture.h"
#include "stb_image.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

#include "VulkanBuffer.h"
#include "VulkanDescriptor.h"
#include "VulkanAllocator.h"

namespace VulkanCore {

	VulkanTexture::VulkanTexture(const std::string& filepath)
		: m_FilePath(filepath)
	{
		CreateTextureImage();
		CreateTextureImageView();
		CreateTextureSampler();
	}

	VulkanTexture::VulkanTexture(VkImage image, VkImageView imageView, bool destroyImg)
		: m_Info{ image, imageView }, m_Release(destroyImg)
	{
		CreateTextureSampler();
	}

	VulkanTexture::VulkanTexture(uint32_t width, uint32_t height)
	{

	}

	VulkanTexture::~VulkanTexture()
	{
		Release();
	}

	void VulkanTexture::CreateTextureImage()
	{
		auto device = VulkanDevice::GetDevice();

		m_Pixels = stbi_load(m_FilePath.c_str(), &m_Width, &m_Height, &m_Channels, STBI_rgb_alpha);

		VkDeviceSize imageSize = m_Width * m_Height * 4;
		VK_CORE_ASSERT(m_Pixels, "Failed to Load Image {0}", m_FilePath);

		VulkanBuffer stagingBuffer{ imageSize, 1, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

		stagingBuffer.Map();
		stagingBuffer.WriteToBuffer(m_Pixels, imageSize);
		stagingBuffer.Unmap();

		free(m_Pixels);

		CreateImage();
		TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		device->CopyBufferToImage(stagingBuffer.GetBuffer(), m_Info.Image, m_Width, m_Height, 1);
		TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void VulkanTexture::CreateImage()
	{
		auto device = VulkanDevice::GetDevice();
		VulkanAllocator allocator("Image2D");

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

		m_Info.MemoryAlloc = allocator.AllocateImage(imageInfo, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, m_Info.Image);
	}

	void VulkanTexture::TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		auto device = VulkanDevice::GetDevice();
		VkCommandBuffer commandBuffer = device->GetCommandBuffer();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_Info.Image;
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

		device->FlushCommandBuffer(commandBuffer);
	}

	void VulkanTexture::CreateTextureSampler()
	{
		auto device = VulkanDevice::GetDevice();

		VkSamplerCreateInfo samplerCreateInfo{};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.anisotropyEnable = VK_TRUE;

		auto deviceProps = device->GetPhysicalDeviceProperties();
		samplerCreateInfo.maxAnisotropy = deviceProps.limits.maxSamplerAnisotropy;

		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 2.0f;

		VK_CHECK_RESULT(vkCreateSampler(device->GetVulkanDevice(), &samplerCreateInfo, nullptr, &m_Info.Sampler), "Failed to Create Texture Sampler!");
	}

	void VulkanTexture::CreateTextureImageView()
	{
		auto device = VulkanDevice::GetDevice();

		VkImageViewCreateInfo imageViewInfo{};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.image = m_Info.Image;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = 1;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;

		VK_CHECK_RESULT(vkCreateImageView(device->GetVulkanDevice(), &imageViewInfo, nullptr, &m_Info.ImageView), "Failed to Create Texture Image View!");
	}

	void VulkanTexture::Release()
	{
		auto device = VulkanDevice::GetDevice();
		VulkanAllocator allocator("Texture2D");

		if (m_Release)
		{
			vkDestroyImageView(device->GetVulkanDevice(), m_Info.ImageView, nullptr);
			allocator.DestroyImage(m_Info.Image, m_Info.MemoryAlloc);
		}

		vkDestroySampler(device->GetVulkanDevice(), m_Info.Sampler, nullptr);
	}

}