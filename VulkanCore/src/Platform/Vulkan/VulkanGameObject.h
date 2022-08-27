#pragma once
#include "VulkanModel.h"
#include "VulkanCore/Core/Components.h"

namespace VulkanCore {

	class VulkanGameObject
	{
	public:
		using entt_id = unsigned int;
		using Map = std::unordered_map<entt_id, VulkanGameObject>;

		static VulkanGameObject CreateGameObject()
		{
			static entt_id currentID = 0;
			return VulkanGameObject{ currentID++ };
		}

		static VulkanGameObject MakePointLight(float intensity = 10.0f, float radius = 0.1f, glm::vec3 color = glm::vec3(1.0f));

		inline entt_id GetObjectID() { return m_ID; }

		std::shared_ptr<VulkanModel> Model;
		glm::vec3 Color;
		TransformComponent Transform{};
		std::unique_ptr<PointLightComponent> PointLight = nullptr;
	private:
		VulkanGameObject(entt_id id)
			: m_ID(id) {}
	private:
		entt_id m_ID;
	};
}