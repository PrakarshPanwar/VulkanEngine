#include "vulkanpch.h"
#include "Platform/Vulkan/VulkanVertexBuffer.h"

namespace VulkanCore {

	std::shared_ptr<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		return std::make_shared<VulkanVertexBuffer>(size);
	}

}
