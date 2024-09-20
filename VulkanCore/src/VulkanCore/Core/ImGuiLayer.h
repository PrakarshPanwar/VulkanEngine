#pragma once
#include "glfw_vulkan.h"
#include "VulkanCore/Core/Layer.h"

#include "Platform/Vulkan/VulkanDescriptor.h"
#include "Platform/Vulkan/VulkanImage.h"
#include "Platform/Vulkan/VulkanTexture.h"
#include "Platform/Vulkan/VulkanRenderCommandBuffer.h"

namespace VulkanCore {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void OnAttach() override;
		void OnDetach() override;
		void ImGuiBegin();
		void ImGuiEnd();
		void ShutDown();

		static VkDescriptorSet AddTexture(const VulkanImage& image);
		static VkDescriptorSet AddTexture(const VulkanTexture& texture);
		static VkDescriptorSet AddTexture(VulkanTextureCube& textureCube);
		static void UpdateDescriptor(VkDescriptorSet descriptorSet, const VulkanImage& image);
		static void UpdateDescriptor(VkDescriptorSet descriptorSet, const VulkanTexture& texture);
		static void UpdateDescriptor(VkDescriptorSet descriptorSet, VulkanTextureCube& textureCube);

		static void CheckVkResult(VkResult error);

		void BlockEvents(bool block) { m_BlockEvents = block; }

		static ImGuiLayer* Get() { return s_Instance; }
		inline bool GetBlockEvents() const { return m_BlockEvents; }
	private:
		void SetDarkThemeColor();
	private:
		std::shared_ptr<VulkanDescriptorPool> m_ImGuiGlobalPool;
		std::shared_ptr<VulkanRenderCommandBuffer> m_ImGuiCmdBuffer;

		bool m_BlockEvents = false;

		static ImGuiLayer* s_Instance;

		friend class VulkanRenderer;
	};

}

#define SHOW_FRAMERATES ImGui::Text("Frame Time: %.3f ms\t Frames Per Second: %.2f FPS", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate)
