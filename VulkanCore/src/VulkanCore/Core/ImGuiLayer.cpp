#include "vulkanpch.h"
#include "ImGuiLayer.h"

#include "Platform/Vulkan/VulkanSwapChain.h"
#include "Application.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <glm/gtx/log_base.hpp>
#include <filesystem>

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

		GLFWwindow* window = (GLFWwindow*)Application::Get()->GetWindow()->GetNativeWindow();
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
		//ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		//VK_CORE_INFO("Current Directory Path: {0}", std::filesystem::current_path());

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

	void ImGuiLayer::ImGuiRenderandEnd(VkCommandBuffer commandBuffer)
	{
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	}

	void ImGuiLayer::ShutDown()
	{
		ImGui_ImplVulkan_Shutdown();
	}

	void ImGuiLayer::CheckVkResult(VkResult error)
	{
		if (error == 0)
			return;

		VK_CORE_ERROR("[ImGui] Error: VkResult = {0}", error);
	}

	void ImGuiLayer::SetDarkThemeColor() // TODO: Colors are very gray in ImGui, need to Re-Color Everything
	{
#define ENABLE_GAMMA 1
		auto& style = ImGui::GetStyle();
		style.WindowBorderSize = 0.0f;

		auto& colors = ImGui::GetStyle().Colors;
#if ENABLE_GAMMA // TODO: Better Names of Color Variables
		glm::vec3 gammaVal = glm::vec3(1 / 0.44f);
		glm::vec3 colorCode1 = glm::pow(glm::vec3(0.11f, 0.105f, 0.11f), gammaVal);
		glm::vec3 colorCode2 = glm::pow(glm::vec3(0.3f, 0.305f, 0.31f), gammaVal);
		glm::vec3 colorCode3 = glm::pow(glm::vec3(0.15f, 0.1505f, 0.151f), gammaVal);
		glm::vec3 colorCode4 = glm::pow(glm::vec3(0.2f, 0.205f, 0.21f), gammaVal);
		glm::vec3 colorCode5 = glm::pow(glm::vec3(0.38f, 0.3805f, 0.381f), gammaVal);
		glm::vec3 colorCode6 = glm::pow(glm::vec3(0.28f, 0.2805f, 0.281f), gammaVal);

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
#else
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.011f, 0.0105f, 0.011f, 1.0f };

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
#endif
	}

}