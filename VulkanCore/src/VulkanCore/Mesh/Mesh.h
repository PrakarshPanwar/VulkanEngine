#pragma once
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanVertexBuffer.h"
#include "Platform/Vulkan/VulkanIndexBuffer.h"

#include <glm/glm.hpp>
#include <assimp/scene.h>

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

	struct MeshBuilder
	{
		std::vector<Vertex> Vertices{};
		std::vector<uint32_t> Indices{};
		int TextureID;

		void LoadMesh(const std::string& filepath);
		void LoadMesh(const std::string& filepath, int texID);
		void LoadMeshFromAssimp(const std::string& filepath, int texID);
		void ProcessNode(aiNode* node, const aiScene* scene);
		void ProcessMesh(aiMesh* mesh, const aiScene* scene);
	};

	class Mesh
	{
	public:
		Mesh() = default;
		Mesh(const MeshBuilder& builder);
		~Mesh();

		Mesh(const Mesh&) = default;

		void Bind(VkCommandBuffer commandBuffer);
		void Draw(VkCommandBuffer commandBuffer);

		void SetFilePath(const std::string& filepath) { m_FilePath = filepath; }

		inline std::string& GetFilePath() { return m_FilePath; }
		inline int GetMaterialID() { return m_MaterialID; }
		inline uint32_t GetVertexCount() const { return m_VertexCount; }
		inline uint32_t GetIndexCount() const { return m_IndexCount; }

		static std::shared_ptr<Mesh> CreateMeshFromFile(const std::string& filepath);
		static std::shared_ptr<Mesh> CreateMeshFromFile(const std::string& filepath, const glm::vec3& modelColor);
		static std::shared_ptr<Mesh> CreateMeshFromFile(const std::string& filepath, int texID);
		static std::shared_ptr<Mesh> CreateMeshFromAssimp(const std::string& filepath, int texID);
	private:
		void CreateVertexBuffers(const std::vector<Vertex>& vertices);
		void CreateIndexBuffers(const std::vector<uint32_t>& indices);
	private:
		std::string m_FilePath;
		std::unique_ptr<VulkanVertexBuffer> m_VertexBuffer;
		std::unique_ptr<VulkanIndexBuffer> m_IndexBuffer;
		uint32_t m_VertexCount, m_IndexCount;
		int m_MaterialID;
		bool m_HasIndexBuffer = false;
	};

}