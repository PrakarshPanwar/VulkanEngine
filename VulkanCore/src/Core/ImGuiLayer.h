#pragma once
#include "glfw_vulkan.h"

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
		//VkDescriptorPool m_ImGuiPool = VK_NULL_HANDLE;
	};

}