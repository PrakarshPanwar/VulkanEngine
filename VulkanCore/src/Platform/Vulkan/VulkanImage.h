#pragma once
#include "VulkanContext.h"

#include <glm/glm.hpp>

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
		RGBA8_UNORM,
		RGBA16_NORM,
		RGBA16_UNORM,
		RGBA16F,
		RGBA32F,
		R11G11B10F,

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
		std::string DebugName;

		uint32_t Width, Height;
		uint32_t Samples = 1;
		uint32_t MipLevels = 1;
		ImageFormat Format;
		ImageUsage Usage;
		bool Transfer = false;
		TextureWrap SamplerWrap = TextureWrap::Clamp;
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
		VulkanImage() = default;
		VulkanImage(const ImageSpecification& spec);
		VulkanImage(uint32_t width, uint32_t height, ImageUsage usage, ImageFormat format);
		~VulkanImage();

		void Invalidate();
		void CreateImageViewSingleMip(uint32_t mip);

		glm::uvec2 GetMipSize(uint32_t mipLevel) const;
		inline const VulkanImageInfo& GetVulkanImageInfo() const { return m_Info; }
		inline const VkDescriptorImageInfo& GetDescriptorInfo() const { return m_DescriptorImageInfo; }
		inline const VkDescriptorImageInfo& GetMipDescriptorInfo(uint32_t mipLevel) const { return m_DescriptorMipImagesInfo.at(mipLevel); }
		inline const ImageSpecification& GetSpecification() const { return m_Specification; }
	private:
		void UpdateImageDescriptor();
		void Release();
	private:
		VulkanImageInfo m_Info;
		VkDescriptorImageInfo m_DescriptorImageInfo{};
		ImageSpecification m_Specification;

		std::vector<VkImageView> m_MipReferences;
		std::unordered_map<uint32_t, VkDescriptorImageInfo> m_DescriptorMipImagesInfo;
	};

}