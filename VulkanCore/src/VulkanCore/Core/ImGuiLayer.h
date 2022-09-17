#pragma once
#include "glfw_vulkan.h"
#include "VulkanCore/Core/Layer.h"

#include "Platform/Vulkan/VulkanDescriptor.h"

#include <glm/glm.hpp>

#define DEPTH_RESOURCES 1

namespace VulkanCore {

	class ImGuiLayer : public Layer // TODO: Remove all images, framebuffers, renderpass from this class 
	{								// Will be managed by class SceneRenderer
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void OnAttach() override;
		void OnDetach() override;
		void ImGuiBegin();
		void ImGuiRenderandEnd(VkCommandBuffer commandBuffer);
		void ShutDown();

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