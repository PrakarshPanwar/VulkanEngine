#include "vulkanpch.h"
#include "ImGuiLayer.h"

#include "Application.h"
#include "Core.h"
#include "VulkanCore/Renderer/Renderer.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanSwapChain.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include "ImGuizmo.h"

#include <glm/glm.hpp>
#include <glm/gtc/color_space.hpp>
#include "optick.h"

#define IMGUI_VIEWPORTS 0

namespace VulkanCore {

	ImGuiLayer* ImGuiLayer::s_Instance;

	ImGuiLayer::ImGuiLayer()
	{
		s_Instance = this;
	}

	ImGuiLayer::~ImGuiLayer()
	{
	}

	void ImGuiLayer::OnAttach()
	{
		auto context = VulkanContext::GetCurrentContext();
		auto device = VulkanContext::GetCurrentDevice();

		DescriptorPoolBuilder descriptorPoolBuilder = {};
		descriptorPoolBuilder.SetPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000);
		descriptorPoolBuilder.AddPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000);

		m_ImGuiGlobalPool = descriptorPoolBuilder.Build();

		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#if IMGUI_VIEWPORTS
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif

		GLFWwindow* window = (GLFWwindow*)Application::Get()->GetWindow()->GetNativeWindow();
		ImGui_ImplGlfw_InitForVulkan(window, true);

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = context->m_VulkanInstance;
		init_info.PhysicalDevice = device->GetPhysicalDevice();
		init_info.Device = device->GetVulkanDevice();
		init_info.Queue = device->GetGraphicsQueue();
		init_info.RenderPass = VulkanSwapChain::GetSwapChain()->GetRenderPass();
		init_info.DescriptorPool = m_ImGuiGlobalPool->GetVulkanDescriptorPool();
		init_info.MinImageCount = 2;
		init_info.ImageCount = 3;
		init_info.CheckVkResultFn = CheckVkResult;
		init_info.MSAASamples = device->GetMSAASampleCount();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		bool initSuccess = ImGui_ImplVulkan_Init(&init_info);
		VK_CORE_ASSERT(initSuccess, "Failed to Initialize ImGui");

#define OPENSANS 0
#define SOURCESANSPRO 1
#if OPENSANS
		io.FontDefault = io.Fonts->AddFontFromFileTTF("fonts/opensans/OpenSans-Regular.ttf", 18.0f);
		io.Fonts->AddFontFromFileTTF("fonts/opensans/OpenSans-Bold.ttf", 18.0f);
#elif SOURCESANSPRO
		io.FontDefault = io.Fonts->AddFontFromFileTTF("fonts/SourceSansPro/SourceSansPro-Regular.ttf", 18.0f);
		io.Fonts->AddFontFromFileTTF("fonts/SourceSansPro/SourceSansPro-Bold.ttf", 18.0f);
#else
		io.FontDefault = io.Fonts->AddFontFromFileTTF("fonts/WorkSans/static/WorkSans-Regular.ttf", 18.0f);
		io.Fonts->AddFontFromFileTTF("fonts/WorkSans/static/WorkSans-Bold.ttf", 18.0f);
#endif

		SetDarkThemeColor();

		bool createFont = ImGui_ImplVulkan_CreateFontsTexture();
		VK_CORE_ASSERT(createFont, "Failed to Create Font Textures");

		ImGui_ImplVulkan_DestroyFontsTexture();
	}

	void ImGuiLayer::OnDetach()
	{
	}

	void ImGuiLayer::ImGuiBegin()
	{
		VK_CORE_PROFILE();

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void ImGuiLayer::ImGuiEnd()
	{
		VK_CORE_PROFILE();

		auto swapChain = VulkanSwapChain::GetSwapChain();
		auto cmdBuffer = VulkanRenderer::Get()->GetRendererCommandBuffer();
		VkCommandBuffer vulkanCmdBuffer = std::static_pointer_cast<VulkanRenderCommandBuffer>(cmdBuffer)->RT_GetActiveCommandBuffer();

#if IMGUI_VIEWPORTS
		ImGuiIO& io = ImGui::GetIO();
		Application* app = Application::Get();
		io.DisplaySize = ImVec2{ (float)app->GetWindow()->GetWidth(), (float)app->GetWindow()->GetHeight() };
#endif

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vulkanCmdBuffer);

#if IMGUI_VIEWPORTS
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
#endif
	}

	void ImGuiLayer::ShutDown()
	{
		auto device = VulkanContext::GetCurrentDevice();
		ImGui_ImplVulkan_Shutdown();
	}

	VkDescriptorSet ImGuiLayer::AddTexture(const VulkanImage& image)
	{
		auto imageDescriptor = image.GetDescriptorImageInfo();

		return ImGui_ImplVulkan_AddTexture(imageDescriptor.sampler,
			imageDescriptor.imageView,
			imageDescriptor.imageLayout);
	}

	VkDescriptorSet ImGuiLayer::AddTexture(const VulkanTexture& texture)
	{
		auto imageDescriptor = texture.GetDescriptorImageInfo();

		return ImGui_ImplVulkan_AddTexture(imageDescriptor.sampler,
			imageDescriptor.imageView,
			imageDescriptor.imageLayout);
	}

	VkDescriptorSet ImGuiLayer::AddTexture(VulkanTextureCube& textureCube)
	{
		VkImageView iconView = textureCube.CreateImageViewPerLayer(0);
		auto imageDescriptor = textureCube.GetDescriptorImageInfo();

		return ImGui_ImplVulkan_AddTexture(imageDescriptor.sampler,
			iconView,
			imageDescriptor.imageLayout);
	}

	VkDescriptorSet ImGuiLayer::AddTexture(VulkanImage& image, uint32_t layer)
	{
		image.CreateImageViewPerLayer(layer);
		auto imageDescriptor = image.GetDescriptorArrayImageInfo(layer);
		
		return ImGui_ImplVulkan_AddTexture(imageDescriptor.sampler,
			imageDescriptor.imageView,
			imageDescriptor.imageLayout);
	}

	void ImGuiLayer::UpdateDescriptor(VkDescriptorSet descriptorSet, const VulkanImage& image)
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkWriteDescriptorSet writeDescriptor{};
		writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptor.dstSet = descriptorSet;
		writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptor.dstBinding = 0;
		writeDescriptor.pImageInfo = &image.GetDescriptorImageInfo();
		writeDescriptor.descriptorCount = 1;
		writeDescriptor.dstArrayElement = 0;

		vkUpdateDescriptorSets(device->GetVulkanDevice(), 1, &writeDescriptor, 0, nullptr);
	}

	void ImGuiLayer::UpdateDescriptor(VkDescriptorSet descriptorSet, const VulkanTexture& texture)
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkWriteDescriptorSet writeDescriptor{};
		writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptor.dstSet = descriptorSet;
		writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptor.dstBinding = 0;
		writeDescriptor.pImageInfo = &texture.GetDescriptorImageInfo();
		writeDescriptor.descriptorCount = 1;
		writeDescriptor.dstArrayElement = 0;

		vkUpdateDescriptorSets(device->GetVulkanDevice(), 1, &writeDescriptor, 0, nullptr);
	}

	void ImGuiLayer::UpdateDescriptor(VkDescriptorSet descriptorSet, VulkanTextureCube& textureCube)
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkImageView iconView = textureCube.CreateImageViewPerLayer(0);
		VkDescriptorImageInfo descriptorInfo = textureCube.GetDescriptorImageInfo();
		descriptorInfo.imageView = iconView;

		VkWriteDescriptorSet writeDescriptor{};
		writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptor.dstSet = descriptorSet;
		writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptor.dstBinding = 0;
		writeDescriptor.pImageInfo = &descriptorInfo;
		writeDescriptor.descriptorCount = 1;
		writeDescriptor.dstArrayElement = 0;

		vkUpdateDescriptorSets(device->GetVulkanDevice(), 1, &writeDescriptor, 0, nullptr);
	}

	void ImGuiLayer::UpdateDescriptor(VkDescriptorSet descriptorSet, VulkanImage& image, uint32_t layer)
	{
		auto device = VulkanContext::GetCurrentDevice();

		image.CreateImageViewPerLayer(layer);
		VkDescriptorImageInfo descriptorInfo = image.GetDescriptorArrayImageInfo(layer);

		VkWriteDescriptorSet writeDescriptor{};
		writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptor.dstSet = descriptorSet;
		writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptor.dstBinding = 0;
		writeDescriptor.pImageInfo = &descriptorInfo;
		writeDescriptor.descriptorCount = 1;
		writeDescriptor.dstArrayElement = 0;

		vkUpdateDescriptorSets(device->GetVulkanDevice(), 1, &writeDescriptor, 0, nullptr);
	}

	void ImGuiLayer::CheckVkResult(VkResult error)
	{
		if (error == 0)
			return;

		VK_CORE_ERROR("[ImGui] Error: VkResult = {0}", (int)error);
	}

	void ImGuiLayer::SetDarkThemeColor()
	{
		auto& style = ImGui::GetStyle();
		style.WindowBorderSize = 0.0f;

		auto& colors = ImGui::GetStyle().Colors;

		// TODO: Better Names of Color Variables or either remove these variables entirely
		bool squareVal = false;
		float gammaVal = squareVal ? 2.0f : 2.2f;

		glm::vec3 colorCode1 = glm::convertSRGBToLinear(glm::vec3(0.11f, 0.105f, 0.11f), gammaVal);
		glm::vec3 colorCode2 = glm::convertSRGBToLinear(glm::vec3(0.3f, 0.305f, 0.31f), gammaVal);
		glm::vec3 colorCode3 = glm::convertSRGBToLinear(glm::vec3(0.15f, 0.1505f, 0.151f), gammaVal);
		glm::vec3 colorCode4 = glm::convertSRGBToLinear(glm::vec3(0.2f, 0.205f, 0.21f), gammaVal);
		glm::vec3 colorCode5 = glm::convertSRGBToLinear(glm::vec3(0.38f, 0.3805f, 0.381f), gammaVal);
		glm::vec3 colorCode6 = glm::convertSRGBToLinear(glm::vec3(0.28f, 0.2805f, 0.281f), gammaVal);
		glm::vec3 colorCode7 = glm::convertSRGBToLinear(glm::vec3(0.08f, 0.08f, 0.08f), gammaVal);

		colors[ImGuiCol_WindowBg] = ImVec4{ colorCode1.x, colorCode1.y, colorCode1.z, 1.0f };

		colors[ImGuiCol_Header] = ImVec4{ colorCode4.x, colorCode4.y, colorCode4.z, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ colorCode2.x, colorCode2.y, colorCode2.z, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ colorCode3.x, colorCode3.y, colorCode3.z, 1.0f };

		colors[ImGuiCol_Button] = ImVec4{ colorCode4.x, colorCode4.y, colorCode4.z, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ colorCode2.x, colorCode2.y, colorCode2.z, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ colorCode3.x, colorCode3.y, colorCode3.z, 1.0f };

		colors[ImGuiCol_FrameBg] = ImVec4{ colorCode4.x, colorCode4.y, colorCode4.z, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ colorCode2.x, colorCode2.y, colorCode2.z, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ colorCode3.x, colorCode3.y, colorCode3.z, 1.0f };

		colors[ImGuiCol_Tab] = ImVec4{ colorCode3.x, colorCode3.y, colorCode3.z, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ colorCode5.x, colorCode5.y, colorCode5.z, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ colorCode6.x, colorCode6.y, colorCode6.z, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ colorCode3.x, colorCode3.y, colorCode3.z, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ colorCode4.x, colorCode4.y, colorCode4.z, 1.0f };

		colors[ImGuiCol_TitleBg] = ImVec4{ colorCode3.x, colorCode3.y, colorCode3.z, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ colorCode3.x, colorCode3.y, colorCode3.z, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ colorCode3.x, colorCode3.y, colorCode3.z, 1.0f };

		colors[ImGuiCol_MenuBarBg] = ImVec4{ colorCode1.x, colorCode1.y, colorCode1.z, 1.0f };
		colors[ImGuiCol_PopupBg] = ImVec4{ colorCode7.x, colorCode7.y, colorCode7.z, 0.97f };
	}

}
