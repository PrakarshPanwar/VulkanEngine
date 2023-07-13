#pragma once
#include "Resource.h"

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
	public:
		virtual bool HasGeometryShader() const = 0;
	};

}