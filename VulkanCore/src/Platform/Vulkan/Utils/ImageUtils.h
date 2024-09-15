#pragma once
#include "ImageUtils.h"

namespace VulkanCore {

	namespace Utils {

		VkFormat VulkanImageFormat(ImageFormat format);
		VkSamplerAddressMode VulkanSamplerWrap(TextureWrap wrap);
		VkSampleCountFlagBits VulkanSampleCount(uint32_t sampleCount);
		uint32_t CalculateMipCount(uint32_t width, uint32_t height);
		bool IsDepthFormat(ImageFormat format);
		bool IsMultisampled(ImageSpecification spec);
		bool IsMultisampled(FramebufferSpecification spec);
		bool IsMultisampled(RenderPassSpecification spec);

		void InsertImageMemoryBarrier(VkCommandBuffer cmdBuf, VkImage image,
			VkAccessFlags2 srcFlags, VkAccessFlags2 dstFlags,
			VkImageLayout oldLayout, VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage,
			VkImageSubresourceRange subresourceRange);

	}

}
