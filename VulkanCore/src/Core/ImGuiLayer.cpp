#include "vulkanpch.h"
#include "ImGuiLayer.h"

#include "VulkanSwapChain.h"
#include "VulkanApplication.h"

#include "Assert.h"
#include "Log.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

namespace VulkanCore {

	ImGuiLayer::ImGuiLayer()
	{

	}

	ImGuiLayer::~ImGuiLayer()
	{

	}

	void ImGuiLayer::OnAttach()
	{
		DescriptorPoolBuilder descriptorPoolBuilder = DescriptorPoolBuilder(*VulkanDevice::GetDevice());
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

		m_GlobalPool = descriptorPoolBuilder.Build();

		ImGui::CreateContext();

		GLFWwindow* window = (GLFWwindow*)VulkanApplication::Get()->GetWindow()->GetNativeWindow();
		ImGui_ImplGlfw_InitForVulkan(window, true);

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = VulkanDevice::GetDevice()->GetVulkanInstance();
		init_info.PhysicalDevice = VulkanDevice::GetDevice()->GetPhysicalDevice();
		init_info.Device = VulkanDevice::GetDevice()->GetVulkanDevice();
		init_info.Queue = VulkanDevice::GetDevice()->GetGraphicsQueue();
		init_info.DescriptorPool = m_GlobalPool->GetDescriptorPool();
		init_info.MinImageCount = 2;
		init_info.ImageCount = 2;
		init_info.CheckVkResultFn = CheckVkResult;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_info, VulkanSwapChain::GetSwapChain()->GetRenderPass());
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO();

		io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Regular.ttf", 18.0f);
		SetDarkThemeColor();

		VkCommandBuffer commandBuffer = VulkanDevice::GetDevice()->BeginSingleTimeCommands();
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
		VulkanDevice::GetDevice()->EndSingleTimeCommands(commandBuffer);
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	void ImGuiLayer::OnDetach()
	{

	}

	void ImGuiLayer::ImGuiBegin()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiLayer::ImGuiRender()
	{
		ImGui::Render();
	}

	void ImGuiLayer::ImGuiEnd(VkCommandBuffer commandBuffer)
	{
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	}

	void ImGuiLayer::ShutDown()
	{
		ImGui_ImplVulkan_Shutdown();
	}

	void ImGuiLayer::UploadImGuiFonts(VkCommandBuffer commandBuffer)
	{
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	}

	void ImGuiLayer::DestroyImGuiFonts()
	{
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	void ImGuiLayer::CheckVkResult(VkResult error)
	{
		if (error == 0)
			return;

		VK_CORE_ERROR("[ImGui] Error: VkResult = {0}", error);
	}

	void ImGuiLayer::SetDarkThemeColor()
	{
		auto& style = ImGui::GetStyle();
		style.WindowBorderSize = 0.0f;

		auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

		colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

		colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	}

}