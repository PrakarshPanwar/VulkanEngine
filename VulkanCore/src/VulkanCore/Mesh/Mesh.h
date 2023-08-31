#pragma once
#include "VulkanCore/Asset/Asset.h"
#include "VulkanCore/Asset/AssetMetadata.h"
#include "VulkanCore/Renderer/Material.h"
#include "VulkanCore/Renderer/VertexBuffer.h"
#include "VulkanCore/Renderer/IndexBuffer.h"

#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define RAY_TRACING 1

#if RAY_TRACING
#define VERTEX_ALIGN alignas(16)
#else
#define VERTEX_ALIGN
#endif

namespace VulkanCore {

	struct VERTEX_ALIGN Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Binormal;
		glm::vec2 TexCoord;
	};

	struct MeshKey
	{
		uint64_t MeshHandle;
		uint64_t MaterialHandle;
		uint32_t SubmeshIndex;

		bool operator==(const MeshKey& other)
		{
			return MeshHandle == other.MeshHandle &&
				SubmeshIndex == other.SubmeshIndex &&
				MaterialHandle == other.MaterialHandle;
		}

		bool operator<(const MeshKey& other) const
		{
			if (MeshHandle < other.MeshHandle)
				return true;

			if (MeshHandle > other.MeshHandle)
				return false;

			if (SubmeshIndex < other.SubmeshIndex)
				return true;

			if (SubmeshIndex > other.SubmeshIndex)
				return false;

			if (MaterialHandle < other.MaterialHandle)
				return true;

			if (MaterialHandle > other.MaterialHandle)
				return false;

			return false;
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

		inline std::shared_ptr<Material> GetMaterial(uint32_t index = 0) const { return m_Materials[index]; }
		// TODO: This method is temporary for now
		inline void SetMaterial(std::shared_ptr<Material> material) { m_Materials[0] = material; }

		inline std::shared_ptr<VertexBuffer> GetVertexBuffer() const { return m_VertexBuffer; }
		inline std::shared_ptr<IndexBuffer> GetIndexBuffer() const { return m_IndexBuffer; }
		inline uint32_t GetVertexCount() const { return (uint32_t)m_Vertices.size(); }
		inline uint32_t GetIndexCount() const { return (uint32_t)m_Indices.size(); }

		inline AssetType GetType() const override { return AssetType::MeshAsset; }
		static AssetType GetStaticType() { return AssetType::MeshAsset; }
	private:
		aiScene* m_Scene;
		std::unique_ptr<Assimp::Importer> m_Importer;

		std::vector<Submesh> m_Submeshes{};
		std::vector<MeshNode> m_Nodes{};

		std::vector<Vertex> m_Vertices{};
		std::vector<uint32_t> m_Indices{};

		// TODO: In future we will support separate material for each submesh
		std::vector<std::shared_ptr<Material>> m_Materials;

		std::shared_ptr<VertexBuffer> m_VertexBuffer;
		std::shared_ptr<IndexBuffer> m_IndexBuffer;

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

		inline AssetType GetType() const override { return AssetType::Mesh; }
		static AssetType GetStaticType() { return AssetType::Mesh; }
	private:
		std::shared_ptr<MeshSource> m_MeshSource;
		std::vector<uint32_t> m_Submeshes;
	};

}
