#pragma once
#include "glfw_vulkan.h"
#include "VulkanCore/Core/Layer.h"

#include "Platform/Vulkan/VulkanDescriptor.h"

#define DEPTH_RESOURCES 1

namespace VulkanCore {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void Init();
		void OnAttach() override;
		void OnDetach() override;
		void ImGuiBegin();
		void ImGuiRenderandEnd(VkCommandBuffer commandBuffer);
		void ShutDown();

		static void CheckVkResult(VkResult error);

		void BlockEvents(bool block) { m_BlockEvents = block; }

		static ImGuiLayer* Get() { return s_Instance; }
		inline bool GetBlockEvents() const { return m_BlockEvents; }
		inline VkCommandBuffer GetCommandBuffers(int index) { return m_ImGuiCommandBuffers[index]; }
		inline VkRenderPass GetImGuiRenderPass() { return m_ViewportRenderPass; }
		inline VkImage GetImage(int index) { return m_ViewportImages[index]; }
		inline VkImageView GetImageView(int index) { return m_ViewportImageViews[index]; }
	private:
		void CreateCommandBuffers();
		void CreateImGuiRenderPass();
		void CreateImGuiFramebuffers();

		void SetDarkThemeColor();
		void CreateImGuiViewportImages();
		void CreateImGuiMultisampleImages();
		void CreateDepthResources();
		void CreateImGuiViewportImageViews();
	private:
		static ImGuiLayer* s_Instance;

		std::unique_ptr<VulkanDescriptorPool> m_GlobalPool;
		std::vector<VkImage> m_ViewportImages;
		std::vector<VkImage> m_DepthImages;
		std::vector<VkImage> m_ColorImages;
		std::vector<VmaAllocation> m_ViewportImageAllocs;
		std::vector<VmaAllocation> m_DepthImageAllocs;
		std::vector<VmaAllocation> m_ColorImageAllocs;
		std::vector<VkImageView> m_ViewportImageViews;
		std::vector<VkImageView> m_DepthImageViews;
		std::vector<VkImageView> m_ColorImageViews;
		std::vector<VkFramebuffer> m_Framebuffers;
		std::vector<VkCommandBuffer> m_ImGuiCommandBuffers;
		VkRenderPass m_ViewportRenderPass;

		bool m_BlockEvents = false;

		// TODO: Maybe this could be changed in the future(Removing VulkanRenderer as friend class)
		friend class VulkanRenderer;
	};

}