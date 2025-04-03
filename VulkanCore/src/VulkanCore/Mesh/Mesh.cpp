#include "vulkanpch.h"
#include "Mesh.h"

#include "VulkanCore/Core/HashCombine.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Timer.h"
#include "VulkanCore/Core/Components.h"
#include "VulkanCore/Asset/AssetManager.h"
#include "Platform/Vulkan/VulkanMaterial.h"

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
			VulkanCore::HashCombine(seed, vertex.Position, vertex.Normal, vertex.TexCoord);
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

	Mesh::Mesh(std::shared_ptr<MeshSource> meshSource)
		: m_MeshSource(meshSource)
	{
		InvalidateSubmeshes();
	}

	Mesh::Mesh()
		: m_MeshSource(std::make_shared<MeshSource>(std::string{}))
	{
	}

	void Mesh::InvalidateSubmeshes()
	{
		for (const auto& meshNode : m_MeshSource->GetMeshNodes())
		{
			for (uint32_t submeshIndex : meshNode.Submeshes)
				m_Submeshes.emplace_back(submeshIndex);
		}
	}

	Mesh::~Mesh()
	{
	}

	std::unique_ptr<Assimp::Importer> MeshSource::s_Importer;

	static const uint32_t s_MeshImportFlags = {
		aiProcess_Triangulate |	          // Use triangles
		aiProcess_CalcTangentSpace |
		aiProcess_JoinIdenticalVertices | // For Index Buffer
		aiProcess_GenUVCoords |           // Generate UV Coords
		aiProcess_GenNormals |            // Generate Normals for Mesh
		aiProcess_GlobalScale |			  // For FBX Mesh scaling
		aiProcess_OptimizeMeshes |
		aiProcess_SortByPType |
		aiProcess_ValidateDataStructure
	};

	MeshSource::MeshSource(const std::string& filepath)
	{
		if (filepath.empty())
			return;

		VK_CORE_INFO("Loading Mesh: {0}", filepath);

		if (!s_Importer)
			s_Importer = std::make_unique<Assimp::Importer>();

		const aiScene* scene = s_Importer->ReadFile(filepath.data(), s_MeshImportFlags);
		if (!scene || !scene->HasMeshes())
		{
			VK_CORE_ERROR("Failed to load Mesh file: {0}", filepath);
			return;
		}

		m_Scene = (aiScene*)scene;
		m_Submeshes.reserve(scene->mNumMeshes);

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
			submesh.MaterialIndex = mesh->mMaterialIndex;

			vertexCount += mesh->mNumVertices;
			indexCount += submesh.IndexCount;
		}

		m_Vertices.reserve(vertexCount);
		m_Indices.reserve(indexCount);

		// Allocating Root Node
		m_Nodes.emplace_back();
	}

	MeshSource::~MeshSource()
	{
	}

}
