#pragma once
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanVertexBuffer.h"
#include "Platform/Vulkan/VulkanIndexBuffer.h"
#include "VulkanCore/Renderer/Material.h"
#include "VulkanCore/Asset/Asset.h"

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
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Binormal;
		glm::vec3 Color;
		glm::vec2 TexCoord;

		bool operator==(const Vertex& other) const
		{
			return Position == other.Position &&
				Color == other.Color && 
				Normal == other.Normal &&
				TexCoord == other.TexCoord;
		}
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
		uint32_t MaterialIndex;
		glm::mat4 LocalTransform;
	};

	class MeshSource
	{
	public:
		MeshSource();
		MeshSource(const std::string& filepath);
		~MeshSource();

		const aiScene* GetAssimpScene() const { return m_Scene; }
		inline const std::vector<MeshNode>& GetMeshNodes() const { return m_Nodes; }
		inline const std::vector<Submesh>& GetSubmeshes() const { return m_Submeshes; }

		inline std::shared_ptr<Material> GetMaterial(uint32_t index = 0) const { return m_Materials[index]; }
		// TODO: This method is temporary for now
		inline void SetMaterial(std::shared_ptr<Material> material) { m_Materials.push_back(material); }

		inline std::shared_ptr<VulkanVertexBuffer> GetVertexBuffer() const { return m_VertexBuffer; }
		inline std::shared_ptr<VulkanIndexBuffer> GetIndexBuffer() const { return m_IndexBuffer; }
		inline uint32_t GetVertexCount() const { return (uint32_t)m_Vertices.size(); }
		inline uint32_t GetIndexCount() const { return (uint32_t)m_Indices.size(); }

		uint64_t GetMeshHandle() const { return m_MeshHandle; }
		std::string& GetFilePath() { return m_FilePath; }
	private:
		std::string m_FilePath;
		uint64_t m_MeshHandle;

		aiScene* m_Scene;
		std::unique_ptr<Assimp::Importer> m_Importer;

		std::vector<Submesh> m_Submeshes{};
		std::vector<MeshNode> m_Nodes{};

		std::vector<Vertex> m_Vertices{};
		std::vector<uint32_t> m_Indices{};

		// TODO: In future we will support separate material for each submesh
		std::vector<std::shared_ptr<Material>> m_Materials;

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

	class Mesh : public Asset
	{
	public:
		Mesh();
		Mesh(std::shared_ptr<MeshSource> meshSource);
		~Mesh();

		void InvalidateSubmeshes();
		inline std::shared_ptr<MeshSource> GetMeshSource() const { return m_MeshSource; }
		inline const std::vector<uint32_t>& GetSubmeshes() const { return m_Submeshes; }

		AssetType GetType() const override { return AssetType::Mesh; }

		static std::shared_ptr<Mesh> LoadMesh(const char* filepath);
		static std::shared_ptr<VulkanVertexBuffer> GetTransformBuffer(uint64_t meshKey) { return s_MeshTransformBuffer[meshKey]; }

		static void ClearAllMeshes();
	private:
		std::shared_ptr<MeshSource> m_MeshSource;
		std::vector<uint32_t> m_Submeshes;
		// TODO: We will not need this once we have VulkanMaterial and MaterialTable

		// Hash => Filepath Hash Value, Value => Transform Storage Buffer Set
		static std::map<uint64_t, std::shared_ptr<MeshSource>> s_MeshSourcesMap;
		static std::map<uint64_t, std::shared_ptr<VulkanVertexBuffer>> s_MeshTransformBuffer;
	};

}