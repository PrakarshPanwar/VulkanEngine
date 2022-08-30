#pragma once
#include "VulkanCore/Core/Layer.h"
#include "VulkanCore/Events/KeyEvent.h"
#include "VulkanCore/Events/MouseEvent.h"
#include "VulkanCore/Scene/Scene.h"
#include "VulkanCore/Systems/RenderSystem.h"
#include "VulkanCore/Systems/PointLightSystem.h"
#include "VulkanCore/Renderer/EditorCamera.h"

#include "Platform/Vulkan/VulkanSwapChain.h"
#include "Platform/Vulkan/VulkanBuffer.h"
#include "Platform/Vulkan/VulkanTexture.h"

namespace VulkanCore {

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		~EditorLayer();

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate() override;
		void OnEvent(Event& e) override;
		void OnImGuiRender() override;
	private:
		bool OnKeyEvent(KeyPressedEvent& keyEvent);
		bool OnMouseScroll(MouseScrolledEvent& mouseScroll);
		void LoadEntities();
	private:
		std::shared_ptr<Scene> m_Scene;
		EditorCamera m_EditorCamera;

		SceneInfo m_SceneRender{}, m_PointLightScene{};
		std::vector<VkDescriptorSet> m_GlobalDescriptorSets{ VulkanSwapChain::MaxFramesInFlight };
		std::vector<std::unique_ptr<VulkanBuffer>> m_UniformBuffers{ VulkanSwapChain::MaxFramesInFlight };

		std::shared_ptr<RenderSystem> m_RenderSystem;
		std::shared_ptr<PointLightSystem> m_PointLightSystem;

		std::shared_ptr<VulkanTexture> m_DiffuseMap, m_NormalMap, m_SpecularMap, m_DiffuseMap2, m_NormalMap2,
			m_SpecularMap2, m_DiffuseMap3, m_NormalMap3, m_SpecularMap3;

		std::shared_ptr<VulkanTexture> m_SwapChainImage;
		VkDescriptorSet m_SwapChainTexID;
		bool m_ImGuiShowWindow = true;
	};

}