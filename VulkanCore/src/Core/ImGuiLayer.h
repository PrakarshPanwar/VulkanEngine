#pragma once
#include "glfw_vulkan.h"
#include "VulkanDescriptor.h"

namespace VulkanCore {

	class ImGuiLayer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void OnAttach();
		void OnDetach();
		void ImGuiBegin();
		void ImGuiRender();
		void ImGuiEnd(VkCommandBuffer commandBuffer);
		void ShutDown();

		void UploadImGuiFonts(VkCommandBuffer commandBuffer);
		void DestroyImGuiFonts();
		static void CheckVkResult(VkResult error);
	private:
		void SetDarkThemeColor();
	private:
		std::unique_ptr<VulkanDescriptorPool> m_GlobalPool;
	};

}