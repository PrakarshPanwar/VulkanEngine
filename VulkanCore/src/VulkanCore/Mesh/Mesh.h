#pragma once
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanVertexBuffer.h"
#include "Platform/Vulkan/VulkanIndexBuffer.h"

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

	class MeshSource
	{
	public:
		MeshSource(const std::string& filepath);
		~MeshSource();
	private:
		std::string m_FilePath;

		aiScene* m_Scene;
		std::unique_ptr<Assimp::Importer> m_Importer;

		std::vector<Vertex> m_Vertices{};
		std::vector<uint32_t> m_Indices{};

		std::shared_ptr<VulkanVertexBuffer> m_VertexBuffer;
		std::shared_ptr<VulkanIndexBuffer> m_IndexBuffer;
	};

	class AssimpMeshImporter
	{
	public:
		static std::shared_ptr<MeshSource> InvalidateMesh();
	};

	class Mesh
	{
	public:
		Mesh() = default;
		Mesh(const std::string& filepath);
		~Mesh();

		Mesh(const Mesh&) = default;
	private:
		std::shared_ptr<MeshSource> m_MeshSource;
		int m_MaterialID;
	};

}