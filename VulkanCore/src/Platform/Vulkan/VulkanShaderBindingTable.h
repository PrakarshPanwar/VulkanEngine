#pragma once
#include "VulkanCore/Renderer/ShaderBindingTable.h"

namespace VulkanCore {

	class VulkanShaderBindingTable : public ShaderBindingTable
	{
	public:
		void SubmitHitMesh();
	};

}
