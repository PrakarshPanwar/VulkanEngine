#include "vulkanpch.h"
#include "SceneRenderer.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"
#include "VulkanCore/Core/Timer.h"
#include "VulkanCore/Renderer/Renderer.h"

#include "Platform/Vulkan/VulkanSwapChain.h"
#include "Platform/Vulkan/VulkanTexture.h"
#include "Platform/Vulkan/VulkanAllocator.h"

namespace VulkanCore {

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

		RenderPassSpecification rpSpec;
		rpSpec.TargetFramebuffer = m_SceneFramebuffer;

		m_SceneRenderPass = std::make_shared<VulkanRenderPass>(rpSpec);
	}

	void SceneRenderer::Release()
	{
		auto device = VulkanContext::GetCurrentDevice();

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
		auto device = VulkanContext::GetCurrentDevice();
		auto swapChain = VulkanSwapChain::GetSwapChain();

		m_SceneCommandBuffers.resize(swapChain->GetImageCount());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = device->GetCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(m_SceneCommandBuffers.size());

		VK_CHECK_RESULT(vkAllocateCommandBuffers(device->GetVulkanDevice(), &allocInfo, m_SceneCommandBuffers.data()), "Failed to Allocate Scene Command Buffers!");

		Renderer::SetCommandBuffers(m_SceneCommandBuffers);
	}

}