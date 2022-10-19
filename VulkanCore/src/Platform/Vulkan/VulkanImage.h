#pragma once
#include "VulkanDevice.h"

namespace VulkanCore {

	namespace Utils {
		void InsertImageMemoryBarrier(VkCommandBuffer cmdBuf, VkImage image,
			VkAccessFlags srcFlags, VkAccessFlags dstFlags,
			VkImageLayout oldLayout, VkImageLayout newLayout,
			VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
			VkImageSubresourceRange subresourceRange);
	}

	enum class ImageFormat
	{
		None,
		RGBA8_SRGB,
		RGBA8_NORM,
		RGBA16F,
		RGBA32F,

		DEPTH24STENCIL8,
		DEPTH16F,
		DEPTH32F
	};

	enum class TextureWrap
	{
		Repeat,
		Clamp
	};

	enum class ImageUsage
	{
		Storage,
		Attachment,
		Texture
	};

	struct ImageSpecification
	{
		uint32_t Width, Height;
		uint32_t Samples = 1;
		uint32_t MipLevels = 1;
		ImageFormat Format;
		ImageUsage Usage;
		TextureWrap SamplerWrap = TextureWrap::Repeat;
	};

	struct VulkanImageInfo
	{
		VkImage Image = nullptr;
		VkImageView ImageView = nullptr;
		VkSampler Sampler = nullptr;
		VmaAllocation MemoryAlloc = nullptr;
	};

	class VulkanImage
	{
	public:
		VulkanImage(const ImageSpecification& spec);
		VulkanImage(uint32_t width, uint32_t height, ImageUsage usage);
		~VulkanImage();

		void Invalidate();

		inline const VulkanImageInfo& GetVulkanImageInfo() const { return m_Info; }
		inline const VkDescriptorImageInfo& GetDescriptorInfo() const { return m_DescriptorImageInfo; }
		inline const ImageSpecification& GetSpecification() const { return m_Specification; }
	private:
		void UpdateImageDescriptor();
		void Release();
	private:
		VulkanImageInfo m_Info;
		VkDescriptorImageInfo m_DescriptorImageInfo{};
		ImageSpecification m_Specification;
	};

}