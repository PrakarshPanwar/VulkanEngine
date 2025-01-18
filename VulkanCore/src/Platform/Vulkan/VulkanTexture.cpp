#include "vulkanpch.h"
#include "VulkanTexture.h"
#include "stb_image.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Renderer.h"

#include "VulkanDescriptor.h"
#include "VulkanAllocator.h"
#include "Utils/ImageUtils.h"

namespace VulkanCore {

	namespace Utils {

		static VkDeviceSize GetMemorySize(ImageFormat format, uint32_t width, uint32_t height)
		{
			switch (format)
			{
			case ImageFormat::R8_UNORM:	   return width * height;
			case ImageFormat::R32I:		   return width * height * sizeof(uint32_t);
			case ImageFormat::R32F:		   return width * height * sizeof(float);
			case ImageFormat::RGBA8_SRGB:  return width * height * 4;
			case ImageFormat::RGBA8_NORM:  return width * height * 4;
			case ImageFormat::RGBA8_UNORM: return width * height * 4;
			case ImageFormat::RGBA16F:	   return width * height * 4 * sizeof(uint16_t);
			case ImageFormat::RGBA32F:	   return width * height * 4 * sizeof(float);
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
			VulkanAllocator allocator("Texture2D");

			// Staging Buffer
			VkBufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = imageSize;
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VkBuffer stagingBuffer;
			VmaAllocation stagingBufferAlloc = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, stagingBuffer);

			uint8_t* dstData = allocator.MapMemory<uint8_t>(stagingBufferAlloc);
			memcpy(dstData, m_LocalStorage, imageSize);
			allocator.UnmapMemory(stagingBufferAlloc);

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
				stagingBuffer,
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
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					subResourceRange);

				device->FlushCommandBuffer(barrierCmd);
			}

			allocator.DestroyBuffer(stagingBuffer, stagingBufferAlloc);
			free(m_LocalStorage);
		}

		m_IsLoaded = true;
	}

	void VulkanTexture::Release()
	{
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
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				subresourceRange);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		subresourceRange.baseMipLevel = mipLevels - 1;

		Utils::InsertImageMemoryBarrier(blitCmd, vulkanImage,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			subresourceRange);

		device->FlushCommandBuffer(blitCmd);
	}

	// ---------------------------------------------------------------------------------------------------
	// -----------------------------TEXTURE CUBE MAPS-----------------------------------------------------
	// ---------------------------------------------------------------------------------------------------

	VulkanTextureCube::VulkanTextureCube(uint32_t width, uint32_t height, ImageFormat format)
		: m_Specification({ width, height, format, TextureWrap::Clamp, true })
	{
	}

	VulkanTextureCube::VulkanTextureCube(void* data, TextureSpecification spec)
		: m_Specification(spec), m_LocalStorage((uint8_t*)data)
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
		VulkanAllocator allocator("TextureCube");

		VkFormat vulkanFormat = Utils::VulkanImageFormat(m_Specification.Format);
		uint32_t mipCount = Utils::CalculateMipCount(m_Specification.Width, m_Specification.Height);

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

		if (m_LocalStorage)
		{
			VkDeviceSize imageSize = Utils::GetMemorySize(m_Specification.Format, m_Specification.Width, m_Specification.Height);

			VkBufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = imageSize * 6;
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VkBuffer stagingBuffer;
			VmaAllocation stagingBufferAlloc = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, stagingBuffer);

			std::array<VkBufferImageCopy, 6> bufferCopyRegions{};
			std::array<VkDeviceSize, 6> byteOffsets{};
			VkDeviceSize byteOffset = 0;

			uint8_t* dstData = allocator.MapMemory<uint8_t>(stagingBufferAlloc);
			memcpy(dstData, m_LocalStorage, imageSize * 6);
			allocator.UnmapMemory(stagingBufferAlloc);

			for (uint32_t i = 0; i < 6; ++i)
			{
				byteOffsets[i] = byteOffset;
				byteOffset += imageSize;
			}

			VkCommandBuffer copyCmd = device->GetCommandBuffer();

			VkImageSubresourceRange subresourceRange{};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 6;

			Utils::InsertImageMemoryBarrier(
				copyCmd, m_Info.Image,
				VK_ACCESS_2_NONE, VK_ACCESS_2_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COPY_BIT,
				subresourceRange);

			for (uint32_t i = 0; i < byteOffsets.size(); ++i)
			{
				VkBufferImageCopy bufferCopyRegion{};
				bufferCopyRegion.bufferOffset = byteOffsets[i];
				bufferCopyRegion.bufferRowLength = 0;
				bufferCopyRegion.bufferImageHeight = 0;

				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = 0;
				bufferCopyRegion.imageSubresource.baseArrayLayer = i;
				bufferCopyRegion.imageSubresource.layerCount = 1;

				bufferCopyRegion.imageOffset = { 0, 0, 0 };
				bufferCopyRegion.imageExtent = { m_Specification.Width, m_Specification.Height, 1 };
				bufferCopyRegions[i] = bufferCopyRegion;
			}

			vkCmdCopyBufferToImage(copyCmd,
				stagingBuffer,
				m_Info.Image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				(uint32_t)bufferCopyRegions.size(),
				bufferCopyRegions.data()
			);

			device->FlushCommandBuffer(copyCmd);

			allocator.DestroyBuffer(stagingBuffer, stagingBufferAlloc);
			free(m_LocalStorage);
		}

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
	
		VkPhysicalDeviceProperties deviceProps = device->GetPhysicalDeviceProperties();
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
			m_LocalStorage ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			subresourceRange);

		device->FlushCommandBuffer(layoutCmd);

		m_DescriptorImageInfo.imageView = m_Info.ImageView;
		m_DescriptorImageInfo.sampler = m_Info.Sampler;
	}

	void VulkanTextureCube::Release()
	{
		Renderer::SubmitResourceFree([mipRefs = m_MipReferences, imageInfo = m_Info]() mutable
		{
			auto device = VulkanContext::GetCurrentDevice();
			VulkanAllocator allocator("TextureCube");

			for (auto& mipRef : mipRefs)
				vkDestroyImageView(device->GetVulkanDevice(), mipRef, nullptr);

			vkDestroyImageView(device->GetVulkanDevice(), imageInfo.ImageView, nullptr);
			vkDestroySampler(device->GetVulkanDevice(), imageInfo.Sampler, nullptr);
			allocator.DestroyImage(imageInfo.Image, imageInfo.MemoryAlloc);
		});
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
				VK_ACCESS_NONE, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_NONE, VK_PIPELINE_STAGE_TRANSFER_BIT,
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
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readonly ? VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			subresourceRange);

		device->FlushCommandBuffer(blitCmd);

		m_DescriptorImageInfo.imageLayout = readonly ? VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
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

	VkImageView VulkanTextureCube::CreateImageViewPerLayer(uint32_t layer)
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkFormat vulkanFormat = Utils::VulkanImageFormat(m_Specification.Format);

		// Create View for single mip
		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.image = m_Info.Image;
		viewCreateInfo.format = vulkanFormat;
		viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = layer;
		viewCreateInfo.subresourceRange.layerCount = 1;

		VkImageView result;
		VK_CHECK_RESULT(vkCreateImageView(device->GetVulkanDevice(), &viewCreateInfo, nullptr, &result), "Failed to Create Image View!");

		m_MipReferences.push_back(result);

		return result;
	}

}