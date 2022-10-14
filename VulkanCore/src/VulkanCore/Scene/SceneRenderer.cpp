#include "vulkanpch.h"
#include "SceneRenderer.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"
#include "VulkanCore/Core/Timer.h"

#include "Platform/Vulkan/VulkanSwapChain.h"
#include "Platform/Vulkan/VulkanTexture.h"
#include "Platform/Vulkan/VulkanAllocator.h"

namespace VulkanCore {

	namespace Utils {

		static VkFormat VulkanImageFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::RGBA8_SRGB:	   return VK_FORMAT_R8G8B8A8_SRGB;
			case ImageFormat::RGBA8_NORM:	   return VK_FORMAT_R8G8B8A8_SNORM;
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

	}

	SceneRenderer* SceneRenderer::s_Instance = nullptr;

	SceneRenderer::SceneRenderer()
	{
		s_Instance = this;
		Init();
	}

	SceneRenderer::~SceneRenderer()
	{
		Release();
	}

	void SceneRenderer::Init()
	{
		CreateCommandBuffers();
		CreateRenderPasswithFramebuffers();
	}

	void SceneRenderer::CreateRenderPasswithFramebuffers()
	{
		std::unique_ptr<Timer> timer = std::make_unique<Timer>("Render Pass Creation");

		FramebufferSpecification fbSpec;
		fbSpec.Width = 1920;
		fbSpec.Height = 1080;
		fbSpec.Attachments = { ImageFormat::RGBA8_SRGB, ImageFormat::DEPTH24STENCIL8 };
		fbSpec.Samples = 8;

		m_ViewportSize = { fbSpec.Width, fbSpec.Height };

		m_SceneFramebuffer = std::make_shared<VulkanFramebuffer>(fbSpec);

		RenderPassSpecification spec;
		spec.TargetFramebuffer = m_SceneFramebuffer;

		m_SceneRenderPass = std::make_shared<VulkanRenderPass>(spec);
	}

	void SceneRenderer::Release()
	{
		auto device = VulkanDevice::GetDevice();

		vkFreeCommandBuffers(device->GetVulkanDevice(), device->GetCommandPool(), 
			static_cast<uint32_t>(m_SceneCommandBuffers.size()), m_SceneCommandBuffers.data());
	}

	void SceneRenderer::RecreateScene()
{
		VK_CORE_INFO("Scene has been Recreated!");		
		m_SceneRenderPass->RecreateFramebuffers(m_ViewportSize.x, m_ViewportSize.y);
	}

	void SceneRenderer::CreateCommandBuffers()
	{
		auto device = VulkanDevice::GetDevice();
		auto swapChain = VulkanSwapChain::GetSwapChain();

		m_SceneCommandBuffers.resize(swapChain->GetImageCount());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = device->GetCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(m_SceneCommandBuffers.size());

		VK_CHECK_RESULT(vkAllocateCommandBuffers(device->GetVulkanDevice(), &allocInfo, m_SceneCommandBuffers.data()), "Failed to Allocate Scene Command Buffers!");
	}

}