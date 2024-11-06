#include "vulkanpch.h"
#include "../VulkanContext.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Image.h"
#include "VulkanCore/Renderer/Framebuffer.h"
#include "VulkanCore/Renderer/RenderPass.h"

namespace VulkanCore {

	namespace Utils {

		VkFormat VulkanImageFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::R8_UNORM:		   return VK_FORMAT_R8_UNORM;
			case ImageFormat::R32I:			   return VK_FORMAT_R32_SINT;
			case ImageFormat::R32F:			   return VK_FORMAT_R32_SFLOAT;
			case ImageFormat::RGBA8_SRGB:	   return VK_FORMAT_R8G8B8A8_SRGB;
			case ImageFormat::RGBA8_NORM:	   return VK_FORMAT_R8G8B8A8_SNORM;
			case ImageFormat::RGBA8_UNORM:	   return VK_FORMAT_R8G8B8A8_UNORM;
			case ImageFormat::RGBA16_NORM:	   return VK_FORMAT_R16G16B16A16_SNORM;
			case ImageFormat::RGBA16_UNORM:	   return VK_FORMAT_R16G16B16A16_UNORM;
			case ImageFormat::RGBA16F:		   return VK_FORMAT_R16G16B16A16_SFLOAT;
			case ImageFormat::RGBA32F:		   return VK_FORMAT_R32G32B32A32_SFLOAT;
			case ImageFormat::R11G11B10F:	   return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
			case ImageFormat::DEPTH24STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
			case ImageFormat::DEPTH16F:		   return VK_FORMAT_D16_UNORM;
			case ImageFormat::DEPTH32F:		   return VK_FORMAT_D32_SFLOAT;
			default:
				VK_CORE_ASSERT(false, "Format not Supported!");
				return (VkFormat)0;
			}
		}

		VkSamplerAddressMode VulkanSamplerWrap(TextureWrap wrap)
		{
			switch (wrap)
			{
			case TextureWrap::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case TextureWrap::Clamp:  return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			default:
				return (VkSamplerAddressMode)0;
			}
		}

		VkSampleCountFlagBits VulkanSampleCount(uint32_t sampleCount)
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

		uint32_t CalculateMipCount(uint32_t width, uint32_t height)
		{
			return (uint32_t)std::_Floor_of_log_2(std::max(width, height)) + 1;
		}

		bool IsDepthFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::DEPTH24STENCIL8: return true;
			case ImageFormat::DEPTH16F:		   return true;
			case ImageFormat::DEPTH32F:		   return true;
			default:						   return false;
			}
		}

		bool IsMultisampled(ImageSpecification spec)
		{
			return spec.Samples > 1;
		}

		bool IsMultisampled(FramebufferSpecification spec)
		{
			return spec.Samples > 1;
		}

		bool IsMultisampled(RenderPassSpecification spec)
		{
			return spec.TargetFramebuffer->GetSpecification().Samples > 1;
		}

		void InsertImageMemoryBarrier(VkCommandBuffer cmdBuf, VkImage image,
			VkAccessFlags2 srcFlags, VkAccessFlags2 dstFlags,
			VkImageLayout oldLayout, VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage,
			VkImageSubresourceRange subresourceRange)
		{
			VkImageMemoryBarrier2 barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange = subresourceRange;
			barrier.srcAccessMask = srcFlags;
			barrier.dstAccessMask = dstFlags;
			barrier.srcStageMask = srcStage;
			barrier.dstStageMask = dstStage;

			VkDependencyInfo dependencyInfo{};
			dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependencyInfo.pImageMemoryBarriers = &barrier;
			dependencyInfo.imageMemoryBarrierCount = 1;

			vkCmdPipelineBarrier2(cmdBuf, &dependencyInfo);
		}

	}

}
