#pragma once
#include <entt.hpp>
#include "VulkanCore/Core/glfw_vulkan.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "VulkanCore/Core/Components.h"
#include "Platform/Vulkan/VulkanDescriptor.h"
#include "Platform/Vulkan/VulkanRenderCommandBuffer.h"

namespace VulkanCore {

	class Entity;
	class SceneRenderer;

	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name);
		void OnUpdateGeometry(SceneRenderer* renderer);
		void OnUpdateLights(std::vector<glm::vec4>& pointLightPositions, std::vector<glm::vec4>& spotLightPositions);
		void UpdatePointLightUB(UBPointLights& ubo);
		void UpdateSpotLightUB(UBSpotLights& ubo);
		void DestroyEntity(Entity entity);
	private:
		entt::registry m_Registry;

		friend class Entity;
		friend class SceneHierarchyPanel;
		friend class SceneSerializer;
		friend class SceneRenderer;
	};

}