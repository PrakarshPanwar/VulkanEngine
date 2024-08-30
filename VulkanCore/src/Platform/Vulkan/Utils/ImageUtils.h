#pragma once
#include "ImageUtils.h"

namespace VulkanCore {

	namespace Utils {

		VkFormat VulkanImageFormat(ImageFormat format);
		bool IsDepthFormat(ImageFormat format);
		VkSamplerAddressMode VulkanSamplerWrap(TextureWrap wrap);
		VkSampleCountFlagBits VulkanSampleCount(uint32_t sampleCount);
		uint32_t CalculateMipCount(uint32_t width, uint32_t height);
		bool IsMultisampled(ImageSpecification spec);
		bool IsMultisampled(FramebufferSpecification spec);
		bool IsMultisampled(RenderPassSpecification spec);

		void InsertImageMemoryBarrier(VkCommandBuffer cmdBuf, VkImage image,
			VkAccessFlags srcFlags, VkAccessFlags dstFlags,
			VkImageLayout oldLayout, VkImageLayout newLayout,
			VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
			VkImageSubresourceRange subresourceRange);

	}

}
