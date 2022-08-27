#pragma once
#include "VulkanCore/Renderer/Camera.h"
#include "Platform/Vulkan/VulkanGameObject.h"
#include "VulkanCore/Scene/Scene.h"

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