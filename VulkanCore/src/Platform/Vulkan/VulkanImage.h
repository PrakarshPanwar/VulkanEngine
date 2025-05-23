#pragma once
#include "VulkanContext.h"
#include "VulkanCore/Renderer/Image.h"

#include <glm/glm.hpp>

namespace VulkanCore {

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
		const VulkanImageInfo& GetVulkanImageInfo() const { return m_Info; }
		const VkDescriptorImageInfo& GetDescriptorImageInfo() const { return m_DescriptorImageInfo; }
		const VkDescriptorImageInfo& GetDescriptorMipImageInfo(uint32_t mipLevel) const { return m_DescriptorMipImagesInfo.at(mipLevel); }
		const VkDescriptorImageInfo& GetDescriptorArrayImageInfo(uint32_t layer) const { return m_DescriptorArrayImagesInfo.at(layer); }
		const ImageSpecification& GetSpecification() const override { return m_Specification; }
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
