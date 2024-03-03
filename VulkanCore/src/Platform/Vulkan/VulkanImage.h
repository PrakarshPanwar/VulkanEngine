#pragma once
#include "VulkanContext.h"
#include "VulkanCore/Renderer/Image.h"

#include <glm/glm.hpp>

namespace VulkanCore {

	namespace Utils {
		void InsertImageMemoryBarrier(VkCommandBuffer cmdBuf, VkImage image,
			VkAccessFlags srcFlags, VkAccessFlags dstFlags,
			VkImageLayout oldLayout, VkImageLayout newLayout,
			VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
			VkImageSubresourceRange subresourceRange);
	}

	struct VulkanImageInfo
	{
		VkImage Image = nullptr;
		VkImageView ImageView = nullptr;
		VkSampler Sampler = nullptr;
		VmaAllocation MemoryAlloc = nullptr;
	};

	class VulkanImage : public Image2D
	{
	public:
		VulkanImage(const ImageSpecification& spec);
		VulkanImage(uint32_t width, uint32_t height, ImageUsage usage, ImageFormat format);
		~VulkanImage();

		void Invalidate();
		void CreateImageViewSingleMip(uint32_t mip);
		void CreateImageViewPerLayer(uint32_t layer);
		void Resize(uint32_t width, uint32_t height, uint32_t mips = 1) override;

		glm::uvec2 GetMipSize(uint32_t mipLevel) const override;
		inline const VulkanImageInfo& GetVulkanImageInfo() const { return m_Info; }
		inline const VkDescriptorImageInfo& GetDescriptorImageInfo() const { return m_DescriptorImageInfo; }
		inline const VkDescriptorImageInfo& GetDescriptorImageInfo(uint32_t mipLevel) const { return m_DescriptorMipImagesInfo.at(mipLevel); }
		inline const ImageSpecification& GetSpecification() const override { return m_Specification; }
	private:
		void UpdateImageDescriptor();
		void Release();
	private:
		VulkanImageInfo m_Info;
		VkDescriptorImageInfo m_DescriptorImageInfo{};
		ImageSpecification m_Specification;

		std::unordered_map<uint32_t, VkDescriptorImageInfo> m_DescriptorMipImagesInfo, m_DescriptorArrayImagesInfo;
	};

}