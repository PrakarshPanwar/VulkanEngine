#pragma once
#include "VulkanCore/Asset/AssetMetadata.h"
#include "VulkanCore/Asset/MaterialAsset.h"
#include "VulkanCore/Renderer/VertexBuffer.h"
#include "VulkanCore/Renderer/IndexBuffer.h"

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
		glm::vec2 TexCoord;

		bool operator==(const Vertex& other) const
		{
			return Position == other.Position &&
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
		int MaterialIndex;
		glm::mat4 LocalTransform;
	};

	class MeshSource : public Asset
	{
	public:
		MeshSource(const std::string& filepath);
		~MeshSource();

		const aiScene* GetAssimpScene() const { return m_Scene; }
		inline const std::vector<MeshNode>& GetMeshNodes() const { return m_Nodes; }
		inline const std::vector<Submesh>& GetSubmeshes() const { return m_Submeshes; }

		inline std::shared_ptr<MaterialAsset> GetBaseMaterial() const { return m_BaseMaterial; }
		inline void SetBaseMaterial(std::shared_ptr<MaterialAsset> baseMaterial) { m_BaseMaterial = baseMaterial; }

		inline std::shared_ptr<VertexBuffer> GetVertexBuffer() const { return m_VertexBuffer; }
		inline std::shared_ptr<IndexBuffer> GetIndexBuffer() const { return m_IndexBuffer; }
		inline uint32_t GetVertexCount() const { return (uint32_t)m_Vertices.size(); }
		inline uint32_t GetIndexCount() const { return (uint32_t)m_Indices.size(); }

		inline AssetType GetType() const override { return AssetType::MeshAsset; }
		static AssetType GetStaticType() { return AssetType::MeshAsset; }
	private:
		aiScene* m_Scene;

		std::vector<Submesh> m_Submeshes{};
		std::vector<MeshNode> m_Nodes{};

		std::vector<Vertex> m_Vertices{};
		std::vector<uint32_t> m_Indices{};

		std::shared_ptr<MaterialAsset> m_BaseMaterial;

		std::shared_ptr<VertexBuffer> m_VertexBuffer;
		std::shared_ptr<IndexBuffer> m_IndexBuffer;

		static std::unique_ptr<Assimp::Importer> s_Importer;

		friend class Mesh;
		friend class MeshImporter;
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
		std::vector<glm::vec3> GetMeshVertices() const;

		inline AssetType GetType() const override { return AssetType::Mesh; }
		static AssetType GetStaticType() { return AssetType::Mesh; }
	private:
		std::shared_ptr<MeshSource> m_MeshSource;
		std::vector<uint32_t> m_Submeshes;
	};

}