#include "vulkanpch.h"
#include "VulkanImage.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"
#include "VulkanAllocator.h"

namespace VulkanCore {

	namespace Utils {

		static VkFormat VulkanImageFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::RGBA8_SRGB:	   return VK_FORMAT_R8G8B8A8_SRGB;
			case ImageFormat::RGBA8_NORM:	   return VK_FORMAT_R8G8B8A8_SNORM;
			case ImageFormat::RGBA16F:		   return VK_FORMAT_R16G16B16A16_SFLOAT;
			case ImageFormat::RGBA32F:		   return VK_FORMAT_R32G32B32A32_SFLOAT;
			case ImageFormat::DEPTH24STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
			case ImageFormat::DEPTH16F:		   return VK_FORMAT_D16_UNORM;
			case ImageFormat::DEPTH32F:		   return VK_FORMAT_D32_SFLOAT;
			default:
				VK_CORE_ASSERT(false, "Format not Supported!");
				return (VkFormat)0;
			}
		}

		static bool IsDepthFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::DEPTH24STENCIL8: return true;
			case ImageFormat::DEPTH16F:		   return true;
			case ImageFormat::DEPTH32F:		   return true;
			default:						   return false;
			}
		}

		static VkSamplerAddressMode VulkanSamplerWrap(TextureWrap wrap)
		{
			switch (wrap)
			{
			case TextureWrap::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case TextureWrap::Clamp:  return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				break;
			default:
				break;
			}
		}

		static VkSampleCountFlagBits VulkanSampleCount(uint32_t sampleCount)
		{
			switch (sampleCount)
			{
			case 1:  return VK_SAMPLE_COUNT_1_BIT;
			case 2:  return VK_SAMPLE_COUNT_2_BIT;
			case 4:  return VK_SAMPLE_COUNT_4_BIT;
			case 8:  return VK_SAMPLE_COUNT_8_BIT;
			case 16: return VK_SAMPLE_COUNT_16_BIT;
			case 32: return VK_SAMPLE_COUNT_32_BIT;
			case 64: return VK_SAMPLE_COUNT_64_BIT;
			default:
				VK_CORE_ASSERT(false, "Sample Bit not Supported! Choose Power of 2");
				return VK_SAMPLE_COUNT_1_BIT;
			}
		}

		static uint32_t CalculateMipCount(uint32_t width, uint32_t height)
		{
			return (uint32_t)std::_Floor_of_log_2(std::max(width, height)) + 1;
		}

		void InsertImageMemoryBarrier(VkCommandBuffer cmdBuf, VkImage image,
			VkAccessFlags srcFlags, VkAccessFlags dstFlags,
			VkImageLayout oldLayout, VkImageLayout newLayout,
			VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
			VkImageSubresourceRange subresourceRange)
		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange = subresourceRange;
			barrier.srcAccessMask = srcFlags;
			barrier.dstAccessMask = dstFlags;

			vkCmdPipelineBarrier(cmdBuf, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}

	}

	VulkanImage::VulkanImage(const ImageSpecification& spec)
		: m_Specification(spec)
	{
	}

	VulkanImage::VulkanImage(uint32_t width, uint32_t height, ImageUsage usage)
	{
		m_Specification.Width = width;
		m_Specification.Height = height;
		m_Specification.Format = ImageFormat::RGBA8_SRGB;
		m_Specification.Usage = usage;

		//Invalidate(); // TODO: For now we have to invalidate manually as copying is involved
	}

	VulkanImage::~VulkanImage()
	{
		if (m_Info.Image == nullptr)
			return;

		Release();
	}

	void VulkanImage::Invalidate()
	{
		if (m_Info.Image != nullptr)
			Release();

		auto device = VulkanDevice::GetDevice();
		VulkanAllocator allocator("Image2D");

		VkFormat vulkanFormat = Utils::VulkanImageFormat(m_Specification.Format);
		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT; // TODO: This shouldn't(probably) be implied

		if (m_Specification.Usage == ImageUsage::Attachment)
		{
			if (Utils::IsDepthFormat(m_Specification.Format))
				usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			else
				usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

		if (m_Specification.Usage == ImageUsage::Texture)
			usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		if (m_Specification.Usage == ImageUsage::Storage)
			usage |= VK_IMAGE_USAGE_STORAGE_BIT;

		VkImageAspectFlags aspectMask = Utils::IsDepthFormat(m_Specification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		if (m_Specification.Format == ImageFormat::DEPTH24STENCIL8)
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

		// Create Vulkan Image
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.format = vulkanFormat;
		imageCreateInfo.extent.width = m_Specification.Width;
		imageCreateInfo.extent.height = m_Specification.Height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = Utils::VulkanSampleCount(m_Specification.Samples);
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = usage;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.mipLevels = 1; // TODO: Add a mips member in 'ImageSpecification'

		m_Info.MemoryAlloc = allocator.AllocateImage(imageCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, m_Info.Image);

		// Create a view for Image
		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.image = m_Info.Image;
		viewCreateInfo.format = vulkanFormat;
		viewCreateInfo.subresourceRange.aspectMask = aspectMask;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;

		VK_CHECK_RESULT(vkCreateImageView(device->GetVulkanDevice(), &viewCreateInfo, nullptr, &m_Info.ImageView), "Failed to Create Image View!");

		VkSamplerAddressMode addressMode = Utils::VulkanSamplerWrap(m_Specification.WrapType);

		// Create a sampler for Image
		VkSamplerCreateInfo sampler{};
		sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler.magFilter = VK_FILTER_LINEAR;
		sampler.minFilter = VK_FILTER_LINEAR;
		sampler.addressModeU = addressMode;
		sampler.addressModeV = addressMode;
		sampler.addressModeW = addressMode;
		sampler.anisotropyEnable = VK_TRUE;

		auto deviceProps = device->GetPhysicalDeviceProperties();
		sampler.maxAnisotropy = deviceProps.limits.maxSamplerAnisotropy;

		sampler.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		sampler.unnormalizedCoordinates = VK_FALSE;
		sampler.compareEnable = VK_FALSE;
		sampler.compareOp = VK_COMPARE_OP_ALWAYS;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.mipLodBias = 0.0f;
		sampler.minLod = 0.0f;
		sampler.maxLod = 2.0f;

		VK_CHECK_RESULT(vkCreateSampler(device->GetVulkanDevice(), &sampler, nullptr, &m_Info.Sampler), "Failed to Create Image Sampler!");
	
		if (m_Specification.Usage == ImageUsage::Storage)
		{
			auto barrierCmd = device->GetCommandBuffer();

			VkImageSubresourceRange subresourceRange{}; // TODO: Add Mips
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;

			Utils::InsertImageMemoryBarrier(barrierCmd, m_Info.Image,
				0, 0,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				subresourceRange);

			device->FlushCommandBuffer(barrierCmd);
		}

		if (m_Specification.Usage == ImageUsage::Texture)
		{
			auto barrierCmd = device->GetCommandBuffer();

			VkImageSubresourceRange subresourceRange{}; // TODO: Add Mips
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount = 1;

			Utils::InsertImageMemoryBarrier(barrierCmd, m_Info.Image,
				0, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				subresourceRange);

			device->FlushCommandBuffer(barrierCmd);
		}

		UpdateImageDescriptor();
	}

	void VulkanImage::UpdateImageDescriptor()
	{
		if (Utils::IsDepthFormat(m_Specification.Format))
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		else if (m_Specification.Usage == ImageUsage::Storage)
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		else
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		m_DescriptorImageInfo.imageView = m_Info.ImageView;
		m_DescriptorImageInfo.sampler = m_Info.Sampler;
	}

	void VulkanImage::Release()
	{
		auto device = VulkanDevice::GetDevice();
		VulkanAllocator allocator("Image2D");

		vkDestroyImageView(device->GetVulkanDevice(), m_Info.ImageView, nullptr);
		vkDestroySampler(device->GetVulkanDevice(), m_Info.Sampler, nullptr);
		allocator.DestroyImage(m_Info.Image, m_Info.MemoryAlloc);
	}

}