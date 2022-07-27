#include "vulkanpch.h"
#include "VulkanGameObject.h"

namespace VulkanCore {

	VulkanGameObject VulkanGameObject::MakePointLight(float intensity, float radius, glm::vec3 color)
	{
		VulkanGameObject gameObject = VulkanGameObject::CreateGameObject();
		gameObject.Color = color;
		gameObject.Transform.Scale.x = radius;
		gameObject.PointLight = std::make_unique<PointLightComponent>();
		gameObject.PointLight->LightIntensity = intensity;

		return gameObject;
	}

}