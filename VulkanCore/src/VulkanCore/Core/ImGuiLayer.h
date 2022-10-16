#pragma once
#include "glfw_vulkan.h"
#include "VulkanCore/Core/Layer.h"

#include "Platform/Vulkan/VulkanDescriptor.h"
#include "Platform/Vulkan/VulkanImage.h"

#include <glm/glm.hpp>

namespace VulkanCore {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void OnAttach() override;
		void OnDetach() override;
		void ImGuiBegin();
		void ImGuiRenderandEnd(VkCommandBuffer commandBuffer);
		void ShutDown();

		static VkDescriptorSet AddTexture(const VulkanImage& Image);
		static void CheckVkResult(VkResult error);

		void BlockEvents(bool block) { m_BlockEvents = block; }

		static ImGuiLayer* Get() { return s_Instance; }
		inline bool GetBlockEvents() const { return m_BlockEvents; }
	private:
		void SetDarkThemeColor();
	private:
		std::unique_ptr<VulkanDescriptorPool> m_GlobalPool;
		bool m_BlockEvents = false;

		static ImGuiLayer* s_Instance;
	};

}

#define SHOW_FRAMERATES ImGui::Text("Application Stats:\n\t Frame Time: %.3f ms\n\t Frames Per Second: %.2f FPS", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate)
