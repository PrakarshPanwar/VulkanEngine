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

		const aiScene* scene = m_Importer->ReadFile(filepath, s_MeshImportFlags);
		if (!scene || !scene->HasMeshes())
		{
			VK_CORE_ERROR("Failed to load Mesh file: {0}", m_FilePath);
			return;
		}

		m_Scene = (aiScene*)scene;


	}

	MeshSource::~MeshSource()
	{
	}

	std::shared_ptr<MeshSource> AssimpMeshImporter::InvalidateMesh()
	{

	}

}