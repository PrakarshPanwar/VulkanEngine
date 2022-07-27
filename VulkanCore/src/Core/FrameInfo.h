#pragma once
#include "Renderer/Camera.h"
#include "VulkanGameObject.h"
#include "Scene/Scene.h"

namespace VulkanCore {

	struct FrameInfo
	{
		int FrameIndex;
		float FrameTime;
		VkCommandBuffer CommandBuffer;
		Camera& GameCamera;
		VkDescriptorSet GlobalDescriptorSet;
		VulkanGameObject::Map& GameObjects;
		std::shared_ptr<Scene> CurrentScene;
	};

}