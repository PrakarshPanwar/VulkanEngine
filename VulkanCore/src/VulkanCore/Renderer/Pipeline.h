#pragma once
#include "Shader.h"
#include "Buffer.h"
#include "RenderPass.h"

namespace VulkanCore {

	enum class PrimitiveTopology
	{
		None,
		LineList,
		TriangleList,
		PatchList
	};

	enum class CompareOp
	{
		None,
		Less,
		LessOrEqual
	};

	enum class CullMode
	{
		None,
		Front,
		Back,
		FrontAndBack
	};

	struct PipelineSpecification
	{
		PipelineSpecification() = default;

		std::string DebugName;
		std::shared_ptr<Shader> pShader;
		std::shared_ptr<RenderPass> pRenderPass;
		PrimitiveTopology Topology = PrimitiveTopology::TriangleList;
		CullMode CullingMode = CullMode::None;
		bool DepthTest = true, DepthWrite = true, DepthClamp = false, Blend = false;
		CompareOp DepthCompareOp = CompareOp::Less;
		uint32_t PatchControlPoints = 0;

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
