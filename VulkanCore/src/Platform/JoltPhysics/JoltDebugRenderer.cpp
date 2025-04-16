#include "vulkanpch.h"
#include "JoltDebugRenderer.h"

#include "Platform/Vulkan/VulkanPipeline.h"

namespace VulkanCore {

	JoltDebugRenderer::JoltDebugRenderer()
	{
	}

	JoltDebugRenderer::~JoltDebugRenderer()
	{
	}

	void JoltDebugRenderer::DrawLine(JPH::RVec3 c0, JPH::RVec3 c1, JPH::Color lineColor)
	{

	}

	void JoltDebugRenderer::DrawTriangle(JPH::RVec3 v0, JPH::RVec3 v1, JPH::RVec3 v2, JPH::Color triColor, ECastShadow castShadow)
	{

	}

	void JoltDebugRenderer::DrawGeometry(JPH::Mat44Arg transformMat, const JPH::AABox& aaBox, float lodScale, JPH::Color geomColor, const GeometryRef& geomRef, ECullMode cullMode, ECastShadow castShadow, EDrawMode drawMode)
	{

	}

	void JoltDebugRenderer::DrawText3D(JPH::RVec3 inPosition, const JPH::string_view& inString, JPH::Color inColor, float inHeight)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(const Triangle* trianglesPtr, int triangleCount)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(const Vertex* verticesPtr, int vertexCount, const JPH::uint32* indicesPtr, int indexCount)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

}
