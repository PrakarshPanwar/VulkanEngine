#include "vulkanpch.h"
#include "JoltDebugRenderer.h"
#include "JoltMesh.h"

#include "VulkanCore/Asset/AssetManager.h"

namespace VulkanCore {

	JoltMesh::JoltMesh(const JPH::DebugRenderer::Vertex* verticesPtr, int vertexCount, const JPH::uint32* indicesPtr, int indexCount)
	{
		std::vector<Vertex> verticesData{};
		std::vector<uint32_t> indicesData{};

		verticesData.reserve(vertexCount);
		indicesData.reserve(indexCount);

		for (int i = 0; i < vertexCount; ++i)
		{
			Vertex& vertexData = verticesData.emplace_back();
			vertexData.Position = { verticesPtr[i].mPosition.x, verticesPtr[i].mPosition.y, verticesPtr[i].mPosition.z };
			vertexData.Normal = { verticesPtr[i].mNormal.x, verticesPtr[i].mNormal.y, verticesPtr[i].mNormal.z };
			vertexData.TexCoord = { verticesPtr[i].mUV.x, verticesPtr[i].mUV.y };
		}

		for (int i = 0; i < indexCount; ++i)
			indicesData.emplace_back(indicesPtr[i]);

		CalculateTangentBasis(verticesData, indicesData);

		// Memory Only Physics Meshes
		auto meshSource = AssetManager::CreateMemoryOnlyAsset<MeshSource>("Box", verticesData, indicesData);
		m_JoltMesh = AssetManager::CreateMemoryOnlyAsset<Mesh>(meshSource);
	}

	void JoltMesh::CalculateTangentBasis(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
	{
		// First, zero out tangents and binormals
		for (auto& vertex : vertices)
		{
			vertex.Tangent = glm::vec3(0.0f);
			vertex.Binormal = glm::vec3(0.0f);
		}

		// Calculate tangents and binormals for each triangle
		for (size_t i = 0; i < indices.size(); i += 3)
		{
			Vertex& v0 = vertices[indices[i]];
			Vertex& v1 = vertices[indices[i + 1]];
			Vertex& v2 = vertices[indices[i + 2]];

			glm::vec3 edge1 = v1.Position - v0.Position;
			glm::vec3 edge2 = v2.Position - v0.Position;

			glm::vec2 deltaUV1 = v1.TexCoord - v0.TexCoord;
			glm::vec2 deltaUV2 = v2.TexCoord - v0.TexCoord;

			float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			glm::vec3 tangent;
			tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

			glm::vec3 binormal;
			binormal.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
			binormal.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
			binormal.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

			// Add to all three vertices of the triangle
			v0.Tangent += tangent;
			v1.Tangent += tangent;
			v2.Tangent += tangent;

			v0.Binormal += binormal;
			v1.Binormal += binormal;
			v2.Binormal += binormal;
		}

		// Normalize and orthogonalize the tangent basis for each vertex
		for (auto& vertex : vertices)
		{
			// Gram-Schmidt orthogonalization
			vertex.Tangent = glm::normalize(vertex.Tangent);
			vertex.Binormal = glm::normalize(vertex.Binormal);
			
			// Make tangent orthogonal to normal
			vertex.Tangent = glm::normalize(vertex.Tangent - vertex.Normal * glm::dot(vertex.Normal, vertex.Tangent));
			
			// Ensure binormal is orthogonal to both normal and tangent
			vertex.Binormal = glm::cross(vertex.Normal, vertex.Tangent);
		}
	}

}
