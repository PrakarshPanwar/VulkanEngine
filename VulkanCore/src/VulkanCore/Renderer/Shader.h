#pragma once
#include <future>
#include "Platform/Vulkan/VulkanDescriptor.h"
#include "../Source/SPIRV-Reflect/spirv_reflect.h"

#define USE_VULKAN_DESCRIPTOR 1

namespace VulkanCore {

	// Based on OpenGL Macros
	enum class ShaderType
	{
		Vertex = 0x8B31,
		Fragment = 0x8B30,
		Geometry = 0x8DD9,
		Compute = 0x91B9
	};

	class Shader
	{

	};

}