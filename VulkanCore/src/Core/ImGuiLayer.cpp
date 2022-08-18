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
		ImGui::CreateContext();

		GLFWwindow* window = (GLFWwindow*)VulkanApplication::Get()->GetWindow()->GetNativeWindow();
		ImGui_ImplGlfw_InitForVulkan(window, true);

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = VulkanDevice::GetDevice()->GetVulkanInstance();
		init_info.PhysicalDevice = VulkanDevice::GetDevice()->GetPhysicalDevice();
		init_info.Device = VulkanDevice::GetDevice()->GetVulkanDevice();
		init_info.Queue = VulkanDevice::GetDevice()->GetGraphicsQueue();
		init_info.DescriptorPool = VulkanApplication::Get()->GetVulkanDescriptorPool()->GetDescriptorPool();
		init_info.MinImageCount = 3;
		init_info.ImageCount = 3;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_info, VulkanSwapChain::GetSwapChain()->GetRenderPass());

		VkCommandBuffer commandBuffer = VulkanDevice::GetDevice()->BeginSingleTimeCommands();
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
		ImGui_ImplVulkan_DestroyFontUploadObjects();
		VulkanDevice::GetDevice()->EndSingleTimeCommands(commandBuffer);
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
		//auto vkDevice = VulkanSwapChain::GetSwapChain()->GetDevice().GetVulkanDevice();
		//vkDestroyDescriptorPool(vkDevice, m_ImGuiPool, nullptr);
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

}