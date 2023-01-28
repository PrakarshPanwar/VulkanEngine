#include "vulkanpch.h"
#include "Mesh.h"

#include "VulkanCore/Core/HashCombine.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Timer.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace std {

	template<>
	struct hash<VulkanCore::Vertex>
	{
		size_t operator()(const VulkanCore::Vertex& vertex) const
		{
			size_t seed = 0;
			VulkanCore::HashCombine(seed, vertex.Position, vertex.Color, vertex.Normal, vertex.TexCoord);
			return seed;
		}
	};

}

namespace VulkanCore {

	namespace Utils {

		static glm::mat4 Mat4FromAIMatrix4(const aiMatrix4x4& matrix)
		{
			glm::mat4 result;

			result[0][0] = matrix.a1; result[1][0] = matrix.a2; result[2][0] = matrix.a3; result[3][0] = matrix.a4;
			result[0][1] = matrix.b1; result[1][1] = matrix.b2; result[2][1] = matrix.b3; result[3][1] = matrix.b4;
			result[0][2] = matrix.c1; result[1][2] = matrix.c2; result[2][2] = matrix.c3; result[3][2] = matrix.c4;
			result[0][3] = matrix.d1; result[1][3] = matrix.d2; result[2][3] = matrix.d3; result[3][3] = matrix.d4;

			return result;
		}

	}

	std::vector<VkVertexInputBindingDescription> Vertex::GetBindingDescriptions()
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> Vertex::GetAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Position) });
		attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Color) });
		attributeDescriptions.push_back({ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Normal) });
		attributeDescriptions.push_back({ 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, TexCoord) });
		attributeDescriptions.push_back({ 4, 0, VK_FORMAT_R32_SINT, offsetof(Vertex, TexID) });

		return attributeDescriptions;
	}

	std::vector<VkVertexInputBindingDescription> QuadVertex::GetBindingDescriptions()
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(QuadVertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> QuadVertex::GetAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(QuadVertex, Position) });
		attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(QuadVertex, TexCoord) });

		return attributeDescriptions;
	}

	Mesh::Mesh(const std::string& filepath)
	{
		m_MeshSource = std::make_shared<MeshSource>(filepath);
		AssimpMeshImporter::TraverseNodes(m_MeshSource, m_MeshSource->GetAssimpScene()->mRootNode, 0);
		AssimpMeshImporter::InvalidateMesh(m_MeshSource);
	}

	std::shared_ptr<Mesh> Mesh::LoadMesh(const char* filepath)
	{
		std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(filepath);
		return mesh;
	}

	Mesh::~Mesh()
	{
	}

	static const uint32_t s_MeshImportFlags = {
		aiProcess_Triangulate |	          // Use triangles
		aiProcess_CalcTangentSpace |
		aiProcess_JoinIdenticalVertices | // For Index Buffer
		aiProcess_GenUVCoords |           // Generate UV Coords
		aiProcess_FlipUVs |
		aiProcess_GenNormals |            // Generate Normals for Mesh
		aiProcess_SortByPType |
		aiProcess_ValidateDataStructure
	};

	MeshSource::MeshSource(const std::string& filepath)
		: m_FilePath(filepath)
	{
		VK_CORE_INFO("Loading Mesh: {0}", filepath);

		m_Importer = std::make_unique<Assimp::Importer>();

		const aiScene* scene = m_Importer->ReadFile(filepath, s_MeshImportFlags);
		if (!scene || !scene->HasMeshes())
		{
			VK_CORE_ERROR("Failed to load Mesh file: {0}", m_FilePath);
			return;
		}

		m_Scene = (aiScene*)scene;

		uint32_t vertexCount = 0, indexCount = 0;
		for (uint32_t m = 0; m < scene->mNumMeshes; ++m)
		{
			aiMesh* mesh = scene->mMeshes[m];

			Submesh& submesh = m_Submeshes.emplace_back();
			submesh.BaseVertex = vertexCount;
			submesh.BaseIndex = indexCount;
			submesh.VertexCount = mesh->mNumVertices;
			submesh.IndexCount = mesh->mNumFaces * 3;
			submesh.MeshName = mesh->mName.C_Str();

			vertexCount += mesh->mNumVertices;
			indexCount += submesh.IndexCount;
		}

		// Allocating Root Node
		m_Nodes.emplace_back();
	}

	MeshSource::~MeshSource()
	{
	}

	void AssimpMeshImporter::InvalidateMesh(std::shared_ptr<MeshSource> meshSource)
	{
		for (uint32_t m = 0; m < (uint32_t)meshSource->m_Submeshes.size(); ++m)
		{
			aiMesh* mesh = meshSource->m_Scene->mMeshes[m];

			for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
			{
				Vertex vertex;
				glm::vec3 mVector;
				mVector.x = mesh->mVertices[i].x;
				mVector.y = mesh->mVertices[i].y;
				mVector.z = mesh->mVertices[i].z;
				vertex.Position = mVector;

				if (mesh->HasNormals())
				{
					mVector.x = mesh->mNormals[i].x;
					mVector.y = mesh->mNormals[i].y;
					mVector.z = mesh->mNormals[i].z;
					vertex.Normal = mVector;
				}

				vertex.Color = glm::vec3{ 1.0f };

				if (mesh->HasTextureCoords(0))
				{
					glm::vec2 mTexCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
					vertex.TexCoord = mTexCoords;
				}

				vertex.TexID = 0;
				meshSource->m_Vertices.push_back(vertex);
			}

			for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
			{
				aiFace face = mesh->mFaces[i];

				for (uint32_t j = 0; j < face.mNumIndices; ++j)
					meshSource->m_Indices.push_back(face.mIndices[j]);
			}
		}

		meshSource->m_VertexBuffer = std::make_shared<VulkanVertexBuffer>(meshSource->m_Vertices.data(), (uint32_t)meshSource->m_Vertices.size());
		meshSource->m_IndexBuffer = std::make_shared<VulkanIndexBuffer>(meshSource->m_Indices.data(), (uint32_t)meshSource->m_Indices.size());
	}

	void AssimpMeshImporter::TraverseNodes(std::shared_ptr<MeshSource> meshSource, aiNode* aNode, uint32_t nodeIndex)
	{
		MeshNode& node = meshSource->m_Nodes[nodeIndex];
		node.Name = aNode->mName.C_Str();
		node.LocalTransform = Utils::Mat4FromAIMatrix4(aNode->mTransformation);

		for (uint32_t i = 0; i < aNode->mNumMeshes; ++i)
		{
			uint32_t submeshIndex = aNode->mMeshes[i];
			auto& submesh = meshSource->m_Submeshes[submeshIndex];
			submesh.NodeName = aNode->mName.C_Str();
			submesh.LocalTransform = node.LocalTransform;

			node.Submeshes.push_back(submeshIndex);
		}

		uint32_t parentNodeIndex = meshSource->m_Nodes.size() - 1;
		node.Children.resize(aNode->mNumChildren);
		for (uint32_t i = 0; i < aNode->mNumChildren; ++i)
		{
			MeshNode& child = meshSource->m_Nodes.emplace_back();
			uint32_t childIndex = meshSource->m_Nodes.size() - 1;
			child.Parent = parentNodeIndex;
			meshSource->m_Nodes[nodeIndex].Children[i] = childIndex;
			TraverseNodes(meshSource, aNode->mChildren[i], childIndex);
		}
	}

#if 0
	void AssimpMeshImporter::ProcessMesh(std::shared_ptr<MeshSource> meshSource, aiMesh* mesh, const aiScene* scene)
	{
		for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
		{
			Vertex vertex;
			glm::vec3 mVector;
			mVector.x = mesh->mVertices[i].x;
			mVector.y = mesh->mVertices[i].y;
			mVector.z = mesh->mVertices[i].z;
			vertex.Position = mVector;

			if (mesh->HasNormals())
			{
				mVector.x = mesh->mNormals[i].x;
				mVector.y = mesh->mNormals[i].y;
				mVector.z = mesh->mNormals[i].z;
				vertex.Normal = mVector;
			}

			vertex.Color = glm::vec3{ 1.0f };

			if (mesh->HasTextureCoords(0))
			{
				glm::vec2 mTexCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
				vertex.TexCoord = mTexCoords;
			}

			vertex.TexID = 0;
			meshSource->m_Vertices.push_back(vertex);
		}

		for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
		{
			aiFace face = mesh->mFaces[i];

			for (uint32_t j = 0; j < face.mNumIndices; ++j)
				meshSource->m_Indices.push_back(face.mIndices[j]);
		}
	}
#endif

}