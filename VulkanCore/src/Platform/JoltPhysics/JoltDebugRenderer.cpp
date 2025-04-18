#include "vulkanpch.h"
#include "JoltDebugRenderer.h"
#include "JoltMesh.h"

#include "VulkanCore/Scene/SceneRenderer.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "Platform/Vulkan/VulkanVertexBuffer.h"

#define MAX_VERTICES 800

namespace VulkanCore {

	JoltDebugRenderer::JoltDebugRenderer()
	{
		Initialize();
		CreateResources();
		CreateMaterials();
	}

	JoltDebugRenderer::~JoltDebugRenderer()
	{
	}

	void JoltDebugRenderer::DrawLine(JPH::RVec3 c0, JPH::RVec3 c1, JPH::Color lineColor)
	{
		LineVertex line0{}, line1{};
		line0.Position = { c0.GetX(), c0.GetY(), c0.GetZ() };
		line1.Position = { c1.GetX(), c1.GetY(), c1.GetZ() };

		line0.Color = { 0.0f, 0.8f, 0.0f, 1.0f };
		line1.Color = { 0.0f, 0.8f, 0.0f, 1.0f };

		m_LinesBuffer.emplace_back(line0);
		m_LinesBuffer.emplace_back(line1);
	}

	void JoltDebugRenderer::DrawTriangle(JPH::RVec3 v0, JPH::RVec3 v1, JPH::RVec3 v2, JPH::Color triColor, ECastShadow castShadow)
	{
		DrawLine(v0, v1, triColor);
		DrawLine(v1, v2, triColor);
		DrawLine(v2, v0, triColor);
	}

	void JoltDebugRenderer::DrawGeometry(JPH::Mat44Arg transformMat, const JPH::AABox& aaBox, float lodScale, JPH::Color geomColor, const GeometryRef& geomRef, ECullMode cullMode, ECastShadow castShadow, EDrawMode drawMode)
	{
		auto sceneRenderer = SceneRenderer::GetSceneRenderer();
		auto& camera = sceneRenderer->GetEditorCamera();
		auto cameraPosition = camera.GetPosition();

		glm::mat4 transform = {
			transformMat.GetColumn4(0).GetX(), transformMat.GetColumn4(0).GetY(), transformMat.GetColumn4(0).GetZ(), transformMat.GetColumn4(0).GetW(),
			transformMat.GetColumn4(1).GetX(), transformMat.GetColumn4(1).GetY(), transformMat.GetColumn4(1).GetZ(), transformMat.GetColumn4(1).GetW(),
			transformMat.GetColumn4(2).GetX(), transformMat.GetColumn4(2).GetY(), transformMat.GetColumn4(2).GetZ(), transformMat.GetColumn4(2).GetW(),
			transformMat.GetColumn4(3).GetX(), transformMat.GetColumn4(3).GetY(), transformMat.GetColumn4(3).GetZ(), transformMat.GetColumn4(3).GetW()
		};

		// Find LOD
		const auto* lod = geomRef->mLODs.data();
		lod = &geomRef->GetLOD(JPH::RVec3{ cameraPosition.x, cameraPosition.y, cameraPosition.z }, aaBox, lodScale);

		// Extract Jolt Mesh
		const JoltMesh* joltMesh = static_cast<const JoltMesh*>(lod->mTriangleBatch.GetPtr());
		sceneRenderer->SubmitPhysicsMesh(joltMesh->GetMesh(), m_DefaultPhysicsMaterialAsset, transform);
	}

	void JoltDebugRenderer::DrawText3D(JPH::RVec3 inPosition, const JPH::string_view& inString, JPH::Color inColor, float inHeight)
	{
		VK_CORE_WARN("Method not Implemented!");
	}

	JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(const Triangle* trianglesPtr, int triangleCount)
	{
		VK_CORE_WARN("Method not Implemented!");
		return nullptr;
	}

	JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(const Vertex* verticesPtr, int vertexCount, const JPH::uint32* indicesPtr, int indexCount)
	{
		JoltMesh* joltMesh = new JoltMesh(verticesPtr, vertexCount, indicesPtr, indexCount);
		return joltMesh;
	}

	void JoltDebugRenderer::FlushData()
	{
		m_LinesVertexBuffer->WriteData(m_LinesBuffer.data(), m_LinesBuffer.size() * sizeof(LineVertex));
	}

	void JoltDebugRenderer::ClearData()
	{
		m_LinesBuffer.clear();
	}

	void JoltDebugRenderer::Draw(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer)
	{
		// Draw Lines
		Renderer::RenderLines(cmdBuffer, m_LinesVertexBuffer, m_LinesBuffer.size());
	}

	void JoltDebugRenderer::CreateResources()
	{
		m_LinesVertexBuffer = std::make_shared<VulkanVertexBuffer>(MAX_VERTICES * sizeof(LineVertex));
	}

	void JoltDebugRenderer::CreateMaterials()
	{
		MaterialData materialData{};
		materialData.Albedo = { 0.2f, 0.3f, 0.8f, 1.0f };
		materialData.Metallic = 0.5f;
		materialData.Roughness = 0.5f;

		m_DefaultPhysicsMaterialAsset = std::make_shared<MaterialAsset>(Material::Create("Default Material"));
		m_DefaultPhysicsMaterialAsset->GetMaterial()->SetMaterialData(materialData);
	}

}
