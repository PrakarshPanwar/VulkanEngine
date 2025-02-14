#include "vulkanpch.h"
#include "VulkanImage.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanAllocator.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "Utils/ImageUtils.h"

namespace VulkanCore {

	VulkanImage::VulkanImage(const ImageSpecification& spec)
		: m_Specification(spec)
	{
	}

	VulkanImage::VulkanImage(uint32_t width, uint32_t height, ImageUsage usage, ImageFormat format)
	{
		m_Specification.Width = width;
		m_Specification.Height = height;
		m_Specification.Format = format;
		m_Specification.Usage = usage;
	}

	VulkanImage::~VulkanImage()
	{
		if (!m_Info.Image)
			return;

		Release();
	}

	void VulkanImage::Invalidate()
	{
		if (m_Info.Image)
			Release();

		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("Image2D");

		bool multisampled = Utils::IsMultisampled(m_Specification);
		VkFormat vulkanFormat = Utils::VulkanImageFormat(m_Specification.Format);
		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT; // TODO: This shouldn't(probably) be implied

		if (m_Specification.Usage == ImageUsage::Attachment || m_Specification.Usage == ImageUsage::ReadAttachment)
		{
			if (Utils::IsDepthFormat(m_Specification.Format))
				usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			else
				usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			if (multisampled)
			{
				usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
				usage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
			}
		}

		if (m_Specification.Transfer || m_Specification.Usage == ImageUsage::Texture)
			usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

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
		imageCreateInfo.extent = { m_Specification.Width, m_Specification.Height, 1 };
		imageCreateInfo.arrayLayers = m_Specification.Layers;
		imageCreateInfo.samples = Utils::VulkanSampleCount(m_Specification.Samples);
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = usage;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.mipLevels = multisampled ? 1 : m_Specification.MipLevels;

		m_Info.MemoryAlloc = allocator.AllocateImage(imageCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, m_Info.Image);
		VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(), VK_OBJECT_TYPE_IMAGE, m_Specification.DebugName, m_Info.Image);

		// Create a view for Image
		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.viewType = m_Specification.Layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.image = m_Info.Image;
		viewCreateInfo.format = vulkanFormat;
		viewCreateInfo.subresourceRange.aspectMask = aspectMask;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = m_Specification.MipLevels;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = m_Specification.Layers;

		VK_CHECK_RESULT(vkCreateImageView(device->GetVulkanDevice(), &viewCreateInfo, nullptr, &m_Info.ImageView), "Failed to Create Image View!");
		VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(), VK_OBJECT_TYPE_IMAGE_VIEW, std::format("{} default image view", m_Specification.DebugName), m_Info.ImageView);

		auto deviceProps = device->GetPhysicalDeviceProperties();
		VkSamplerAddressMode addressMode = Utils::VulkanSamplerWrap(m_Specification.SamplerWrap);
		VkSamplerMipmapMode mipmapMode = Utils::VulkanMipmapMode(m_Specification.SamplerFilter);
		VkFilter filterMode = Utils::VulkanFilterMode(m_Specification.SamplerFilter);

		// Create a sampler for Image
		VkSamplerCreateInfo sampler{};
		sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler.magFilter = filterMode;
		sampler.minFilter = filterMode;
		sampler.addressModeU = addressMode;
		sampler.addressModeV = addressMode;
		sampler.addressModeW = addressMode;
		sampler.anisotropyEnable = VK_TRUE;
		sampler.maxAnisotropy = deviceProps.limits.maxSamplerAnisotropy;
		sampler.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		sampler.unnormalizedCoordinates = VK_FALSE;
		sampler.compareEnable = VK_FALSE;
		sampler.compareOp = VK_COMPARE_OP_ALWAYS;
		sampler.mipmapMode = mipmapMode;
		sampler.mipLodBias = 0.0f;
		sampler.minLod = 0.0f;
		sampler.maxLod = (float)m_Specification.MipLevels;

		VK_CHECK_RESULT(vkCreateSampler(device->GetVulkanDevice(), &sampler, nullptr, &m_Info.Sampler), "Failed to Create Image Sampler!");
		VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(), VK_OBJECT_TYPE_SAMPLER, std::format("{} default image sampler", m_Specification.DebugName), m_Info.Sampler);

		if (m_Specification.Usage == ImageUsage::Storage)
		{
			auto barrierCmd = device->GetCommandBuffer();

			VkImageSubresourceRange subresourceRange{};
			subresourceRange.aspectMask = aspectMask;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = m_Specification.MipLevels;
			subresourceRange.layerCount = m_Specification.Layers;

			Utils::InsertImageMemoryBarrier(barrierCmd, m_Info.Image,
				0, 0,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				subresourceRange);

			device->FlushCommandBuffer(barrierCmd);
		}

		if (m_Specification.Usage == ImageUsage::ReadAttachment)
		{
			auto barrierCmd = device->GetCommandBuffer();

			VkImageSubresourceRange subresourceRange{};
			subresourceRange.aspectMask = aspectMask;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = m_Specification.MipLevels;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount = m_Specification.Layers;

			Utils::InsertImageMemoryBarrier(barrierCmd, m_Info.Image,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				subresourceRange);

			device->FlushCommandBuffer(barrierCmd);
		}

		if (m_Specification.Usage == ImageUsage::Texture)
		{
			auto barrierCmd = device->GetCommandBuffer();

			VkImageSubresourceRange subresourceRange{};
			subresourceRange.aspectMask = aspectMask;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = m_Specification.MipLevels;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount = m_Specification.Layers;

			Utils::InsertImageMemoryBarrier(barrierCmd, m_Info.Image,
				VK_ACCESS_2_NONE, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_TRANSFER_BIT,
				subresourceRange);

			device->FlushCommandBuffer(barrierCmd);
		}

		UpdateImageDescriptor();
	}

	void VulkanImage::CreateImageViewSingleMip(uint32_t mip)
	{
		auto device = VulkanContext::GetCurrentDevice();

		if (m_DescriptorMipImagesInfo.contains(mip))
			return;

		VkFormat vulkanFormat = Utils::VulkanImageFormat(m_Specification.Format);

		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.image = m_Info.Image;
		viewCreateInfo.format = vulkanFormat;
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = mip;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;

		VkImageView result;
		VK_CHECK_RESULT(vkCreateImageView(device->GetVulkanDevice(), &viewCreateInfo, nullptr, &result), "Failed to Create Image View!");
		
		VkDescriptorImageInfo mipImageInfo{};
		mipImageInfo.imageView = result;
		mipImageInfo.imageLayout = m_DescriptorImageInfo.imageLayout;
		mipImageInfo.sampler = m_DescriptorImageInfo.sampler;
		m_DescriptorMipImagesInfo[mip] = mipImageInfo;
	}

	void VulkanImage::CreateImageViewPerLayer(uint32_t layer)
	{
		auto device = VulkanContext::GetCurrentDevice();

		if (m_DescriptorArrayImagesInfo.contains(layer))
			return;

		VkFormat vulkanFormat = Utils::VulkanImageFormat(m_Specification.Format);

		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.image = m_Info.Image;
		viewCreateInfo.format = vulkanFormat;
		viewCreateInfo.subresourceRange.aspectMask = Utils::IsDepthFormat(m_Specification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = layer;
		viewCreateInfo.subresourceRange.layerCount = 1;

		VkImageView result;
		VK_CHECK_RESULT(vkCreateImageView(device->GetVulkanDevice(), &viewCreateInfo, nullptr, &result), "Failed to Create Image View!");

		VkDescriptorImageInfo layerImageInfo{};
		layerImageInfo.imageView = result;
		layerImageInfo.imageLayout = m_DescriptorImageInfo.imageLayout;
		layerImageInfo.sampler = m_DescriptorImageInfo.sampler;
		m_DescriptorArrayImagesInfo[layer] = layerImageInfo;
	}

	void VulkanImage::Resize(uint32_t width, uint32_t height, uint32_t mips)
	{
		m_Specification.Width = width;
		m_Specification.Height = height;
		m_Specification.MipLevels = mips;

		Invalidate();
	}

	glm::uvec2 VulkanImage::GetMipSize(uint32_t mipLevel) const
	{
		uint32_t width = m_Specification.Width, height = m_Specification.Height;
		while (mipLevel != 0)
		{
			width /= 2;
			height /= 2;
			--mipLevel;
		}

		return { width, height };
	}

	void VulkanImage::UpdateImageDescriptor()
	{
		if (m_Specification.Usage == ImageUsage::Storage)
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		else
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;

		m_DescriptorImageInfo.imageView = m_Info.ImageView;
		m_DescriptorImageInfo.sampler = m_Info.Sampler;
	}

	void VulkanImage::Release()
	{
		Renderer::SubmitResourceFree([mipRefs = m_DescriptorMipImagesInfo, layerRefs = m_DescriptorArrayImagesInfo,
			imageInfo = m_Info]() mutable
		{
			auto device = VulkanContext::GetCurrentDevice();
			VulkanAllocator allocator("Image2D");

			vkDestroyImageView(device->GetVulkanDevice(), imageInfo.ImageView, nullptr);
			vkDestroySampler(device->GetVulkanDevice(), imageInfo.Sampler, nullptr);
			allocator.DestroyImage(imageInfo.Image, imageInfo.MemoryAlloc);

			for (auto& [mipID, mipRef] : mipRefs)
				vkDestroyImageView(device->GetVulkanDevice(), mipRef.imageView, nullptr);

			for (auto& [layerID, layerRef] : layerRefs)
				vkDestroyImageView(device->GetVulkanDevice(), layerRef.imageView, nullptr);
		});

		m_DescriptorMipImagesInfo.clear();
		m_DescriptorArrayImagesInfo.clear();
	}

}