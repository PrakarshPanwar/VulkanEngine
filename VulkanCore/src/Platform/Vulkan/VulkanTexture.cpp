#include "vulkanpch.h"
#include "VulkanTexture.h"
#include "stb_image.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Renderer.h"

#include "VulkanBuffer.h"
#include "VulkanDescriptor.h"
#include "VulkanAllocator.h"

namespace VulkanCore {

	namespace Utils {

		static VkFormat VulkanImageFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::RGBA8_SRGB:	   return VK_FORMAT_R8G8B8A8_SRGB;
			case ImageFormat::RGBA8_NORM:	   return VK_FORMAT_R8G8B8A8_SNORM;
			case ImageFormat::RGBA8_UNORM:	   return VK_FORMAT_R8G8B8A8_UNORM;
			case ImageFormat::RGBA16_NORM:	   return VK_FORMAT_R16G16B16A16_SNORM;
			case ImageFormat::RGBA16_UNORM:	   return VK_FORMAT_R16G16B16A16_UNORM;
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

		static VkSamplerAddressMode VulkanSamplerWrap(TextureWrap wrap)
		{
			switch (wrap)
			{
			case TextureWrap::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case TextureWrap::Clamp:  return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			default:
				return (VkSamplerAddressMode)0;
			}
		}

		static uint32_t CalculateMipCount(uint32_t width, uint32_t height)
		{
			return (uint32_t)std::_Floor_of_log_2(std::max(width, height)) + 1;
		}

		static VkDeviceSize GetMemorySize(ImageFormat format, uint32_t width, uint32_t height)
		{
			switch (format)
			{
			case ImageFormat::RGBA8_SRGB: return width * height * 4;
			case ImageFormat::RGBA8_NORM: return width * height * 4;
			case ImageFormat::RGBA8_UNORM: return width * height * 4;
			case ImageFormat::RGBA16F: return width * height * 4 * sizeof(uint16_t);
			case ImageFormat::RGBA32F: return width * height * 4 * sizeof(float);
			default:
				VK_CORE_ASSERT(false, "Format not supported!");
				return 0;
			}
		}

		static void SetImageLayout(VkCommandBuffer layoutCmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange)
		{
			Utils::InsertImageMemoryBarrier(layoutCmd, image, 0, 0,
				oldLayout, newLayout,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				subresourceRange);
		}

	}

	VulkanTexture::VulkanTexture(uint32_t width, uint32_t height, ImageFormat format)
	{
		m_Specification.Width = width;
		m_Specification.Height = height;
		m_Specification.Format = format;

		Invalidate();
	}

	VulkanTexture::VulkanTexture(void* data, TextureSpecification spec)
		: m_Specification(spec), m_LocalStorage((uint8_t*)data)
	{
		Invalidate();
	}

	VulkanTexture::~VulkanTexture()
	{
		if (m_Image->GetDescriptorImageInfo().imageView == nullptr)
			return;

		Release();
	}

	void VulkanTexture::Reload(ImageFormat format)
	{
		m_Specification.Format = format;

		Invalidate();
	}

	void VulkanTexture::Invalidate()
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkDeviceSize imageSize = Utils::GetMemorySize(m_Specification.Format, m_Specification.Width, m_Specification.Height);

		ImageSpecification spec{};
		spec.DebugName = std::format("Texture: ({0}, {1})", m_Specification.Width, m_Specification.Height);
		spec.Width = m_Specification.Width;
		spec.Height = m_Specification.Height;
		spec.Usage = ImageUsage::Texture;
		spec.Format = m_Specification.Format;
		spec.MipLevels = m_Specification.GenerateMips ? Utils::CalculateMipCount(m_Specification.Width, m_Specification.Height) : 1;
		m_Image = std::make_shared<VulkanImage>(spec);
		m_Image->Invalidate();
		m_Info = m_Image->GetVulkanImageInfo();

		if (m_LocalStorage)
		{
			VulkanBuffer stagingBuffer{ imageSize, 1, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

			stagingBuffer.Map();
			stagingBuffer.WriteToBuffer(m_LocalStorage, imageSize);
			stagingBuffer.Unmap();

			VkCommandBuffer copyCmd = device->GetCommandBuffer();
			VkImageSubresourceRange subResourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = { (uint32_t)m_Specification.Width, (uint32_t)m_Specification.Height, 1 };

			vkCmdCopyBufferToImage(copyCmd,
				stagingBuffer.GetBuffer(),
				m_Image->GetVulkanImageInfo().Image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&region);

			device->FlushCommandBuffer(copyCmd);

			if (m_Specification.GenerateMips)
				GenerateMipMaps();

			else
			{
				VkCommandBuffer barrierCmd = device->GetCommandBuffer();

				Utils::InsertImageMemoryBarrier(barrierCmd, m_Image->GetVulkanImageInfo().Image,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					subResourceRange);

				device->FlushCommandBuffer(barrierCmd);
			}
		}

		m_IsLoaded = true;
	}

	void VulkanTexture::Release()
	{
		if (m_LocalStorage)
			free(m_LocalStorage);
	}

	void VulkanTexture::GenerateMipMaps()
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkCommandBuffer blitCmd = device->GetCommandBuffer();

		const VkImage vulkanImage = m_Image->GetVulkanImageInfo().Image;
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.layerCount = 1;
		subresourceRange.levelCount = 1;

		int32_t mipWidth = m_Specification.Width;
		int32_t mipHeight = m_Specification.Height;

		const uint32_t mipLevels = Utils::CalculateMipCount(m_Specification.Width, m_Specification.Height);

		for (uint32_t i = 1; i < mipLevels; ++i)
		{
			subresourceRange.baseMipLevel = i - 1;

			Utils::InsertImageMemoryBarrier(blitCmd, vulkanImage,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
				subresourceRange);

			VkImageBlit imageBlit{};
			imageBlit.srcOffsets[0] = { 0, 0, 0 };
			imageBlit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.srcSubresource.mipLevel = i - 1;
			imageBlit.srcSubresource.baseArrayLayer = 0;
			imageBlit.srcSubresource.layerCount = 1;
			imageBlit.dstOffsets[0] = { 0, 0, 0 };
			imageBlit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.dstSubresource.mipLevel = i;
			imageBlit.dstSubresource.baseArrayLayer = 0;
			imageBlit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(blitCmd,
				vulkanImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				vulkanImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &imageBlit,
				VK_FILTER_LINEAR);

			Utils::InsertImageMemoryBarrier(blitCmd, vulkanImage,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				subresourceRange);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		subresourceRange.baseMipLevel = mipLevels - 1;

		Utils::InsertImageMemoryBarrier(blitCmd, vulkanImage,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			subresourceRange);

		device->FlushCommandBuffer(blitCmd);
	}

	// ---------------------------------------------------------------------------------------------------
	// -----------------------------TEXTURE CUBE MAPS-----------------------------------------------------
	// ---------------------------------------------------------------------------------------------------

	VulkanTextureCube::VulkanTextureCube(uint32_t width, uint32_t height, ImageFormat format)
	{
		m_Specification.Width = width;
		m_Specification.Height = height;
		m_Specification.Format = format;
		m_Specification.SamplerWrap = TextureWrap::Clamp;
		m_Specification.GenerateMips = true;
	}

	VulkanTextureCube::VulkanTextureCube(void* data, TextureSpecification spec)
		: m_Specification(spec)
	{
	}

	VulkanTextureCube::~VulkanTextureCube()
	{
		if (m_Info.Image)
			Release();
	}

	void VulkanTextureCube::Invalidate()
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkFormat vulkanFormat = Utils::VulkanImageFormat(m_Specification.Format);

		uint32_t mipCount = Utils::CalculateMipCount(m_Specification.Width, m_Specification.Height);

		VulkanAllocator allocator("TextureCube");

		// Create Vulkan Image
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.format = vulkanFormat;
		imageCreateInfo.extent = { m_Specification.Width, m_Specification.Height, 1 };
		imageCreateInfo.arrayLayers = 6;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.mipLevels = mipCount;

		m_Info.MemoryAlloc = allocator.AllocateImage(imageCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, m_Info.Image);
		VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(), VK_OBJECT_TYPE_IMAGE, "Default TextureCube", m_Info.Image);

		m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		// Create a view for Image
		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		viewCreateInfo.image = m_Info.Image;
		viewCreateInfo.format = vulkanFormat;
		viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 6;
		viewCreateInfo.subresourceRange.levelCount = mipCount;
		
		VK_CHECK_RESULT(vkCreateImageView(device->GetVulkanDevice(), &viewCreateInfo, nullptr, &m_Info.ImageView), "Failed to Create Cubemap Image View!");
	
		VkSamplerAddressMode addressMode = Utils::VulkanSamplerWrap(m_Specification.SamplerWrap);

		// Create a sampler for Cubemap
		VkSamplerCreateInfo sampler{};
		sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler.magFilter = VK_FILTER_LINEAR;
		sampler.minFilter = VK_FILTER_LINEAR;
		sampler.addressModeU = addressMode;
		sampler.addressModeV = addressMode;
		sampler.addressModeW = addressMode;
		sampler.anisotropyEnable = VK_TRUE;

		VkPhysicalDeviceProperties deviceProps = device->GetPhysicalDeviceProperties();
		sampler.maxAnisotropy = deviceProps.limits.maxSamplerAnisotropy;

		sampler.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		sampler.unnormalizedCoordinates = VK_FALSE;
		sampler.compareEnable = VK_FALSE;
		sampler.compareOp = VK_COMPARE_OP_ALWAYS;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.mipLodBias = 0.0f;
		sampler.minLod = 0.0f;
		sampler.maxLod = (float)mipCount;

		VK_CHECK_RESULT(vkCreateSampler(device->GetVulkanDevice(), &sampler, nullptr, &m_Info.Sampler), "Failed to Create Cubemap Image Sampler!");

		// Set Image to General Layout
		VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipCount, 0, 6 };
		VkCommandBuffer layoutCmd = device->GetCommandBuffer();

		Utils::SetImageLayout(
			layoutCmd, m_Info.Image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			subresourceRange);

		device->FlushCommandBuffer(layoutCmd);

		m_DescriptorImageInfo.imageView = m_Info.ImageView;
		m_DescriptorImageInfo.sampler = m_Info.Sampler;
	}

	// TODO: Should be done in RenderThread
	void VulkanTextureCube::Release()
	{
		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("TextureCube");

		for (auto& mipReference : m_MipReferences)
			vkDestroyImageView(device->GetVulkanDevice(), mipReference, nullptr);

		vkDestroyImageView(device->GetVulkanDevice(), m_Info.ImageView, nullptr);
		vkDestroySampler(device->GetVulkanDevice(), m_Info.Sampler, nullptr);
		allocator.DestroyImage(m_Info.Image, m_Info.MemoryAlloc);
	}

	// NOTE: It should not be called inside Invalidate
	void VulkanTextureCube::GenerateMipMaps(bool readonly)
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkCommandBuffer blitCmd = device->GetCommandBuffer();

		uint32_t mipLevels = Utils::CalculateMipCount(m_Specification.Width, m_Specification.Height);
		for (uint32_t face = 0; face < 6; ++face)
		{
			VkImageSubresourceRange mipSubRange{};
			mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			mipSubRange.baseMipLevel = 0;
			mipSubRange.baseArrayLayer = face;
			mipSubRange.levelCount = 1;
			mipSubRange.layerCount = 1;

			Utils::InsertImageMemoryBarrier(blitCmd, m_Info.Image,
				0, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				mipSubRange);
		}

		for (uint32_t i = 1; i < mipLevels; ++i)
		{
			for (uint32_t face = 0; face < 6; ++face)
			{
				VkImageBlit imageBlit{};

				// Source
				imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.srcSubresource.layerCount = 1;
				imageBlit.srcSubresource.mipLevel = i - 1;
				imageBlit.srcSubresource.baseArrayLayer = face;
				imageBlit.srcOffsets[1].x = int32_t(m_Specification.Width >> (i - 1));
				imageBlit.srcOffsets[1].y = int32_t(m_Specification.Height >> (i - 1));
				imageBlit.srcOffsets[1].z = 1;
				
				// Destination
				imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.dstSubresource.layerCount = 1;
				imageBlit.dstSubresource.mipLevel = i;
				imageBlit.dstSubresource.baseArrayLayer = face;
				imageBlit.dstOffsets[1].x = int32_t(m_Specification.Width >> i);
				imageBlit.dstOffsets[1].y = int32_t(m_Specification.Height >> i);
				imageBlit.dstOffsets[1].z = 1;

				VkImageSubresourceRange mipSubRange{};
				mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				mipSubRange.baseMipLevel = i;
				mipSubRange.baseArrayLayer = face;
				mipSubRange.levelCount = 1;
				mipSubRange.layerCount = 1;

				Utils::InsertImageMemoryBarrier(blitCmd, m_Info.Image,
					0, VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					mipSubRange);

				vkCmdBlitImage(blitCmd,
					m_Info.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					m_Info.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &imageBlit,
					VK_FILTER_LINEAR);

				Utils::InsertImageMemoryBarrier(blitCmd, m_Info.Image,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					mipSubRange);
			}
		}

		VkImageSubresourceRange subresourceRange{};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.layerCount = 6;
		subresourceRange.levelCount = mipLevels;

		Utils::InsertImageMemoryBarrier(blitCmd, m_Info.Image,
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readonly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			subresourceRange);

		device->FlushCommandBuffer(blitCmd);

		m_DescriptorImageInfo.imageLayout = readonly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
	}

	VkImageView VulkanTextureCube::CreateImageViewSingleMip(uint32_t mip)
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkFormat vulkanFormat = Utils::VulkanImageFormat(m_Specification.Format);

		// Create View for single mip
		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		viewCreateInfo.image = m_Info.Image;
		viewCreateInfo.format = vulkanFormat;
		viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = mip;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 6;

		VkImageView result;
		VK_CHECK_RESULT(vkCreateImageView(device->GetVulkanDevice(), &viewCreateInfo, nullptr, &result), "Failed to Create Image View!");

		m_MipReferences.push_back(result);

		return result;
	}

}