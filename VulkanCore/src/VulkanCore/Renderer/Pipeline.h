#pragma once
#include "Shader.h"
#include "Buffer.h"
#include "RenderPass.h"

namespace VulkanCore {

	struct PipelineSpecification
	{
		PipelineSpecification() = default;

		std::string DebugName;
		std::shared_ptr<Shader> pShader;
		std::shared_ptr<RenderPass> pRenderPass;
		bool BackfaceCulling = false;
		bool DepthTest = true;
		bool DepthWrite = true;
		bool Blend = false;
		VertexBufferLayout Layout{};
		VertexBufferLayout InstanceLayout{};
	};

	class Pipeline : public Resource
	{
	public:
		virtual PipelineSpecification GetSpecification() const = 0;
	};

}
