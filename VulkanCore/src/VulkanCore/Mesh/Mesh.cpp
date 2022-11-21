#include "vulkanpch.h"
#include "Mesh.h"

#include "VulkanCore/Core/HashCombine.h"

#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"
#include "VulkanCore/Core/Timer.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <format>

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

	Mesh::Mesh(const MeshBuilder& builder)
	{
		CreateVertexBuffers(builder.Vertices);
		CreateIndexBuffers(builder.Indices);
	}

	Mesh::~Mesh()
	{
	}

	void Mesh::Bind(VkCommandBuffer commandBuffer)
	{
		VkBuffer buffers[] = { m_VertexBuffer->GetBuffer() };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

		if (m_HasIndexBuffer)
			vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}

	void Mesh::Draw(VkCommandBuffer commandBuffer)
	{
		if (m_HasIndexBuffer)
			vkCmdDrawIndexed(commandBuffer, m_IndexCount, 1, 0, 0, 0);

		else
			vkCmdDraw(commandBuffer, m_VertexCount, 1, 0, 0);
	}

	std::shared_ptr<Mesh> Mesh::CreateMeshFromFile(const std::string& filepath)
	{
		MeshBuilder builder{};
		builder.LoadMesh(filepath);

		std::filesystem::path modelFilepath = filepath;
		//std::filesystem::path materialFilePath = modelFilepath.replace_extension(".mtl");

		VK_CORE_TRACE("Loading Model: {0}", modelFilepath.filename());
		VK_CORE_TRACE("\tVertex Count: {0}", builder.Vertices.size());
		VK_CORE_TRACE("\tIndex Count: {0}", builder.Indices.size());
		return std::make_shared<Mesh>(builder);
	}

	std::shared_ptr<Mesh> Mesh::CreateMeshFromFile(const std::string& filepath, const glm::vec3& modelColor)
	{
		return nullptr;
	}

	std::shared_ptr<Mesh> Mesh::CreateMeshFromFile(const std::string& filepath, int texID)
	{
		MeshBuilder builder{};
		builder.LoadMesh(filepath, texID);

		std::filesystem::path modelFilepath = filepath;
		//std::filesystem::path materialFilePath = modelFilepath.replace_extension(".mtl");

		VK_CORE_TRACE("Loading Model: {0}", modelFilepath.filename());
		VK_CORE_TRACE("\tVertex Count: {0}", builder.Vertices.size());
		VK_CORE_TRACE("\tIndex Count: {0}", builder.Indices.size());

		std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(builder);
		mesh->m_FilePath = filepath;
		return mesh;
	}

	std::shared_ptr<Mesh> Mesh::CreateMeshFromAssimp(const std::string& filepath, int texID)
	{
		MeshBuilder builder{};
		std::filesystem::path modelFilepath = filepath;
		Timer timer(std::format("\tProcessing Mesh {0}", modelFilepath.filename().string()));

		builder.LoadMeshFromAssimp(filepath, texID);

		VK_CORE_TRACE("Loading Model: {0}", filepath);
		VK_CORE_TRACE("\tVertex Count: {0}", builder.Vertices.size());
		VK_CORE_TRACE("\tIndex Count: {0}", builder.Indices.size());

		std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(builder);
		mesh->m_FilePath = filepath;
		mesh->m_MaterialID = texID;

		return mesh;
	}

	void Mesh::CreateVertexBuffers(const std::vector<Vertex>& vertices)
	{
		m_VertexCount = (uint32_t)vertices.size();
		VK_CORE_ASSERT(m_VertexCount >= 3, "Vertex Count should be at least greater than or equal to 3!");

		VkDeviceSize bufferSize = sizeof(vertices[0]) * m_VertexCount;
		uint32_t vertexSize = sizeof(vertices[0]);

		VulkanBuffer stagingBuffer{ vertexSize, m_VertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

		stagingBuffer.Map();
		stagingBuffer.WriteToBuffer((void*)vertices.data());

		m_VertexBuffer = std::make_unique<VulkanBuffer>(vertexSize, m_VertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		auto device = VulkanContext::GetCurrentDevice();

		device->CopyBuffer(stagingBuffer.GetBuffer(), m_VertexBuffer->GetBuffer(), bufferSize);
	}

	void Mesh::CreateIndexBuffers(const std::vector<uint32_t>& indices)
	{
		m_IndexCount = (uint32_t)indices.size();
		m_HasIndexBuffer = m_IndexCount > 0;

		if (!m_HasIndexBuffer)
			return;

		VkDeviceSize bufferSize = sizeof(indices[0]) * m_IndexCount;
		uint32_t indexSize = sizeof(indices[0]);

		VulkanBuffer stagingBuffer{ indexSize, m_IndexCount, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

		stagingBuffer.Map();
		stagingBuffer.WriteToBuffer((void*)indices.data());

		m_IndexBuffer = std::make_unique<VulkanBuffer>(indexSize, m_IndexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		auto device = VulkanContext::GetCurrentDevice();

		device->CopyBuffer(stagingBuffer.GetBuffer(), m_IndexBuffer->GetBuffer(), bufferSize);
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

	void MeshBuilder::LoadMesh(const std::string& filepath)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		VK_CORE_ASSERT(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str()), warn + err);

		Vertices.clear();
		Indices.clear();

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex{};

				if (index.vertex_index >= 0)
				{
					vertex.Position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2] };

					vertex.Color = {
					attrib.colors[3 * index.vertex_index + 0],
					attrib.colors[3 * index.vertex_index + 1],
					attrib.colors[3 * index.vertex_index + 2] };
				}

				if (index.normal_index >= 0)
				{
					vertex.Normal = { 
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2] };
				}

				if (index.texcoord_index >= 0)
				{
					vertex.TexCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					attrib.texcoords[2 * index.texcoord_index + 1] };
				}

				vertex.TexID = 0;

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(Vertices.size());
					Vertices.push_back(vertex);
				}

				Indices.push_back(uniqueVertices[vertex]);
			}
		}
	}

	void MeshBuilder::LoadMesh(const std::string& filepath, int texID)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		VK_CORE_ASSERT(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str()), warn + err);

		Vertices.clear();
		Indices.clear();

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex{};

				if (index.vertex_index >= 0)
				{
					vertex.Position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2] };

					vertex.Color = {
					attrib.colors[3 * index.vertex_index + 0],
					attrib.colors[3 * index.vertex_index + 1],
					attrib.colors[3 * index.vertex_index + 2] };
				}

				if (index.normal_index >= 0)
				{
					vertex.Normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2] };
				}

				if (index.texcoord_index >= 0)
				{
					vertex.TexCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					attrib.texcoords[2 * index.texcoord_index + 1] };
				}

				vertex.TexID = texID;

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(Vertices.size());
					Vertices.push_back(vertex);
				}

				Indices.push_back(uniqueVertices[vertex]);
			}
		}
	}

	void MeshBuilder::LoadMeshFromAssimp(const std::string& filepath, int texID)
	{
		const uint32_t s_MeshImportFlags = {
			aiProcess_Triangulate |           // Use triangles
			aiProcess_CalcTangentSpace |
			aiProcess_JoinIdenticalVertices | // For Index Buffer
			aiProcess_GenUVCoords |           // Generate UV Coords
			aiProcess_GenNormals |            // Generate Normals for Mesh
			aiProcess_SortByPType |
			aiProcess_ValidateDataStructure
		};

		Assimp::Importer mImporter{};
		const aiScene* mScene = mImporter.ReadFile(filepath, s_MeshImportFlags);

		VK_CORE_ASSERT(mScene && !(mScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && mScene->mRootNode, mImporter.GetErrorString());

		Vertices.clear();
		Indices.clear();
		TextureID = texID;

		ProcessNode(mScene->mRootNode, mScene);
	}

	void MeshBuilder::ProcessNode(aiNode* node, const aiScene* scene)
	{
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			ProcessMesh(mesh, scene);
		}

		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene);
		}
	}

	void MeshBuilder::ProcessMesh(aiMesh* mesh, const aiScene* scene)
	{
		for (uint32_t i = 0; i < mesh->mNumVertices; i++)
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

			vertex.TexID = TextureID;

			Vertices.push_back(vertex);
		}

		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];

			for (uint32_t j = 0; j < face.mNumIndices; j++)
				Indices.push_back(face.mIndices[j]);
		}
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

}