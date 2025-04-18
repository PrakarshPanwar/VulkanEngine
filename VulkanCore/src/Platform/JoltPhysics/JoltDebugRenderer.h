#pragma once
#include "Jolt/Jolt.h"

#define JPH_DEBUG_RENDERER_EXPORT
#include "Jolt/Renderer/DebugRenderer.h"
#undef JPH_DEBUG_RENDERER_EXPORT

#include "VulkanCore/Renderer/VertexBuffer.h"
#include "VulkanCore/Scene/PhysicsDebugRenderer.h"
#include "VulkanCore/Asset/MaterialAsset.h"

#include <glm/glm.hpp>

namespace VulkanCore {

	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
	};

	class JoltDebugRenderer : virtual public JPH::DebugRenderer, virtual public PhysicsDebugRenderer
	{
	public:
		JoltDebugRenderer();
		~JoltDebugRenderer();

		// These will be called from Jolt
		void DrawLine(JPH::RVec3 c0, JPH::RVec3 c1, JPH::Color lineColor) override;
		void DrawTriangle(JPH::RVec3 v0, JPH::RVec3 v1, JPH::RVec3 v2, JPH::Color triColor, ECastShadow castShadow = ECastShadow::Off) override;
		void DrawGeometry(JPH::Mat44Arg transformMat, const JPH::AABox& aaBox, float lodScale, JPH::Color geomColor, const GeometryRef& geomRef, ECullMode cullMode = ECullMode::CullBackFace, ECastShadow castShadow = ECastShadow::On, EDrawMode drawMode = EDrawMode::Solid) override;
		void DrawText3D(JPH::RVec3 inPosition, const JPH::string_view& inString, JPH::Color inColor = JPH::Color::sWhite, float inHeight = 0.5f) override;
		Batch CreateTriangleBatch(const Triangle* trianglesPtr, int triangleCount) override;
		Batch CreateTriangleBatch(const Vertex* verticesPtr, int vertexCount, const JPH::uint32* indicesPtr, int indexCount) override;

		// These will be called from VulkanCore
		void FlushData() override;
		void ClearData() override;
		void Draw(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer) override;
	private:
		void CreateResources();
		void CreateMaterials();
	private:
		std::shared_ptr<MaterialAsset> m_DefaultPhysicsMaterialAsset;

		std::vector<LineVertex> m_LinesBuffer;
		std::shared_ptr<VertexBuffer> m_LinesVertexBuffer;
	};

}
