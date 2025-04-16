#pragma once
#include "Jolt/Jolt.h"

#define JPH_DEBUG_RENDERER_EXPORT
#include "Jolt/Renderer/DebugRenderer.h"
#undef JPH_DEBUG_RENDERER_EXPORT

#include "VulkanCore/Renderer/Pipeline.h"

namespace VulkanCore {

	class JoltDebugRenderer : JPH::DebugRenderer
	{
	public:
		JoltDebugRenderer();
		~JoltDebugRenderer();

		void DrawLine(JPH::RVec3 c0, JPH::RVec3 c1, JPH::Color lineColor) override;
		void DrawTriangle(JPH::RVec3 v0, JPH::RVec3 v1, JPH::RVec3 v2, JPH::Color triColor, ECastShadow castShadow = ECastShadow::Off) override;
		void DrawGeometry(JPH::Mat44Arg transformMat, const JPH::AABox& aaBox, float lodScale, JPH::Color geomColor, const GeometryRef& geomRef, ECullMode cullMode = ECullMode::CullBackFace, ECastShadow castShadow = ECastShadow::On, EDrawMode drawMode = EDrawMode::Solid) override;
		void DrawText3D(JPH::RVec3 inPosition, const JPH::string_view& inString, JPH::Color inColor = JPH::Color::sWhite, float inHeight = 0.5f) override;
		Batch CreateTriangleBatch(const Triangle* trianglesPtr, int triangleCount) override;
		Batch CreateTriangleBatch(const Vertex* verticesPtr, int vertexCount, const JPH::uint32* indicesPtr, int indexCount) override;
	private:
		std::shared_ptr<Pipeline> m_Line2DPipeline;
		std::shared_ptr<Pipeline> m_PhysicsGeomPipeline;
	};

}
