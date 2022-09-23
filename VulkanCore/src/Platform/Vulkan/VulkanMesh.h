#pragma once
#include "VulkanDevice.h"
#include "VulkanBuffer.h"

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
			return Position == other.Position && Color == other.Color && Normal == other.Normal && TexCoord == other.TexCoord;
		}
	};


	struct ModelBuilder
	{
		std::vector<Vertex> Vertices{};
		std::vector<uint32_t> Indices{};
		int TextureID;

		void LoadModel(const std::string& filepath);
		void LoadModel(const std::string& filepath, int texID);
		void LoadModelFromAssimp(const std::string& filepath, int texID);
		void ProcessNode(aiNode* node, const aiScene* scene);
		void ProcessMesh(aiMesh* mesh, const aiScene* scene);
	};

	class VulkanMesh
	{
	public:
		VulkanMesh() = default;
		VulkanMesh(VulkanDevice& device, const ModelBuilder& builder);
		~VulkanMesh();

		VulkanMesh(const VulkanMesh&) = default;

		void Bind(VkCommandBuffer commandBuffer);
		void Draw(VkCommandBuffer commandBuffer);

		static std::shared_ptr<VulkanMesh> CreateModelFromFile(VulkanDevice& device, const std::string& filepath);
		static std::shared_ptr<VulkanMesh> CreateModelFromFile(VulkanDevice& device, const std::string& filepath, const glm::vec3& modelColor);
		static std::shared_ptr<VulkanMesh> CreateModelFromFile(VulkanDevice& device, const std::string& filepath, int texID);
		static std::shared_ptr<VulkanMesh> CreateModelFromAssimp(VulkanDevice& device, const std::string& filepath, int texID);
	private:
		void CreateVertexBuffers(const std::vector<Vertex>& vertices);
		void CreateIndexBuffers(const std::vector<uint32_t>& indices);
	private:
		VulkanDevice& m_VulkanDevice;
		std::shared_ptr<VulkanBuffer> m_VertexBuffer;
		std::shared_ptr<VulkanBuffer> m_IndexBuffer;
		uint32_t m_VertexCount, m_IndexCount;
		bool m_HasIndexBuffer = false;
	};

}