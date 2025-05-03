#pragma once
#include "Resource.h"

#include <bitset>

namespace VulkanCore {

	// Based on OpenGL Macros
	enum class ShaderType
	{
		Vertex = 0x8B31,
		Fragment = 0x8B30,
		Geometry = 0x8DD9,
		TessellationControl = 0x8E88,
		TessellationEvaluation = 0x8E87,
		Compute = 0x91B9
	};

	class Shader
	{
	public:
		virtual void Reload() = 0;
		virtual bool HasGeometryShader() const = 0;
		virtual bool HasTessellationShaders() const = 0;
	public:
		virtual void SetReloadFlag() { m_ReloadFlag = true; }
		virtual void ResetReloadFlag() { m_ReloadFlag = false; }
		virtual bool GetReloadFlag() { return m_ReloadFlag; }
	protected:
		bool m_ReloadFlag = false;
		std::bitset<12> m_LoadShaderFlag = 0; // 6 divisions of 2 bits, Low = Shader, High = Cache
	};

}
