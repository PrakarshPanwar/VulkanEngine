#pragma once
#include "Resource.h"

namespace VulkanCore {

	// Based on Vulkan enums
	enum class ShaderType
	{
		// Standard Types
		Vertex = 0x1,
		Geometry = 0x8,
		Fragment = 0x10,
		Compute = 0x20,

		// Ray Tracing Types
		RayGeneration = 0x100,
		RayAnyHit = 0x200,
		RayClosestHit = 0x400,
		RayMiss = 0x800,
		RayIntersection = 0x1000
	};

	class Shader
	{
	public:
		virtual void Reload() = 0;
	public:
		virtual void SetReloadFlag() { m_ReloadFlag = true; }
		virtual void ResetReloadFlag() { m_ReloadFlag = false; }
		virtual bool GetReloadFlag() { return m_ReloadFlag; }
	protected:
		bool m_ReloadFlag = false;
	};

}
