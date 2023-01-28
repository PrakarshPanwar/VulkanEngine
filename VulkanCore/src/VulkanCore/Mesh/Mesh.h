#pragma once
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanVertexBuffer.h"
#include "Platform/Vulkan/VulkanIndexBuffer.h"
#include "Platform/Vulkan/VulkanStorageBuffer.h"

// TODO: This include should be in PCH
#include <map>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace VulkanCore {

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Color;
		glm::vec3 Normal;
		glm::vec2 TexCoord;
		int TexID;

		static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

		bool operator==(const Vertex& other) const
		{
			return Position == other.Position && Color == other.Color && 
				Normal == other.Normal && TexCoord == other.TexCoord;
		}
	};

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec2 TexCoord;

		static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
	};

	struct MeshNode
	{
		std::string Name;
		uint32_t Parent;
		std::vector<uint32_t> Submeshes;
		std::vector<uint32_t> Children;
		glm::mat4 LocalTransform;
	};

	struct Submesh
	{
		std::string MeshName, NodeName;
		uint32_t BaseVertex, BaseIndex;
		uint32_t VertexCount, IndexCount;
		glm::mat4 LocalTransform;
	};

	class MeshSource
	{
	public:
		MeshSource(const std::string& filepath);
		~MeshSource();

		const aiScene* GetAssimpScene() const { return m_Scene; }
		uint64_t GetMeshKey() const { return m_MeshKey; }
		std::string& GetFilePath() { return m_FilePath; }
	private:
		std::string m_FilePath;

		aiScene* m_Scene;
		std::unique_ptr<Assimp::Importer> m_Importer;
		uint64_t m_MeshKey;

		std::vector<Submesh> m_Submeshes;
		std::vector<MeshNode> m_Nodes;

		std::vector<Vertex> m_Vertices{};
		std::vector<uint32_t> m_Indices{};

		std::shared_ptr<VulkanVertexBuffer> m_VertexBuffer;
		std::shared_ptr<VulkanIndexBuffer> m_IndexBuffer;

		friend class Mesh;
		friend class AssimpMeshImporter;
	};

	class AssimpMeshImporter
	{
	public:
		static void InvalidateMesh(std::shared_ptr<MeshSource> meshSource);
		static void TraverseNodes(std::shared_ptr<MeshSource> meshSource, aiNode* aNode, uint32_t nodeIndex);
		static void ProcessMesh(std::shared_ptr<MeshSource> meshSource, aiMesh* mesh, const aiScene* scene);
	};

	class Mesh
	{
	public:
		Mesh() = default;
		Mesh(const std::string& filepath);
		~Mesh();

		inline std::shared_ptr<MeshSource> GetMeshSource() const { return m_MeshSource; }
		inline uint32_t GetVertexCount() const { return (uint32_t)m_MeshSource->m_Vertices.size(); }
		inline uint32_t GetIndexCount() const { return (uint32_t)m_MeshSource->m_Indices.size(); }

		static std::shared_ptr<Mesh> LoadMesh(const char* filepath);

	private:
		std::shared_ptr<MeshSource> m_MeshSource;
		// TODO: In we will not need this once we have VulkanMaterial and MaterialTable
		int m_MaterialID;

		// Hash => Filepath Hash Value, Value => Transform Storage Buffer Set
		static std::map<uint64_t, std::shared_ptr<MeshSource>> s_MeshSourcesMap;
		static std::map<uint64_t, std::vector<VulkanStorageBuffer>> s_MeshTransformBuffer;
	};

}