#pragma once
#include "Shader.h"
#include "Buffer.h"
#include "RenderPass.h"

namespace VulkanCore {

	enum class CompareOp
	{
		None,
		Less,
		LessOrEqual
	};

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
		CompareOp DepthCompareOp = CompareOp::Less;

		VertexBufferLayout Layout{};
		VertexBufferLayout InstanceLayout{};
	};

	class Pipeline : public Resource
	{
	public:
		virtual void ReloadPipeline() = 0;
		virtual PipelineSpecification GetSpecification() const = 0;
	};

}
