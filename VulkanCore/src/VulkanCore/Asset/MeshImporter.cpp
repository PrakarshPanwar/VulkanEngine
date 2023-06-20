#include "vulkanpch.h"
#include "MeshImporter.h"
#include "AssetManager.h"

#include <yaml-cpp/yaml.h>

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

	std::shared_ptr<MeshSource> MeshImporter::ImportAssimpMesh(AssetHandle handle, const AssetMetadata& metadata)
	{
		std::shared_ptr<MeshSource> meshSource = std::make_shared<MeshSource>(metadata);

		TraverseNodes(meshSource, meshSource->m_Scene->mRootNode, 0);
		InvalidateMesh(meshSource);

		return meshSource;
	}

	// NOTE: For this we have to serialize/deserialize mesh asset which includes
	// Mesh Asset handle(Most Important) and Submesh Indices(Optional/In future)
	std::shared_ptr<Mesh> MeshImporter::ImportMesh(AssetHandle handle, const AssetMetadata& metadata)
	{
		std::shared_ptr<Asset> mesh = nullptr;

		bool loaded = DeserializeMesh(metadata, mesh);
		if (loaded)
			return std::static_pointer_cast<Mesh>(mesh);

		return nullptr;
	}

	// NOTE: Asset in parameter is of type Mesh(not MeshSource)
	void MeshImporter::SerializeMesh(const AssetMetadata& metadata, std::shared_ptr<Asset> asset)
	{
		std::string filepath = metadata.FilePath.string();
		// Obtain MeshSource
		std::shared_ptr<Mesh> mesh = std::dynamic_pointer_cast<Mesh>(asset);
		std::shared_ptr<MeshSource> meshSource = mesh->GetMeshSource();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Mesh";
		out << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "MeshAsset" << YAML::Value << meshSource->Handle;
		out << YAML::EndMap;
		out << YAML::EndMap; // End Root Node

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	bool MeshImporter::DeserializeMesh(const AssetMetadata& metadata, std::shared_ptr<Asset>& asset)
	{
		std::string filepath = metadata.FilePath.string();
		std::ifstream stream(filepath);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Mesh"])
			return false;

		YAML::Node rootNode = data["Mesh"];
		if (!rootNode["MeshAsset"])
			return false;

		AssetHandle meshSourceHandle = rootNode["MeshAsset"].as<uint64_t>();
		std::shared_ptr<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(meshSourceHandle);

		// NOTE: We have to find a way to set handle
		std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(meshSource);
		asset = mesh;
		return true;
	}

	void MeshImporter::InvalidateMesh(std::shared_ptr<MeshSource> meshSource)
	{
		for (uint32_t m = 0; m < (uint32_t)meshSource->m_Submeshes.size(); ++m)
		{
			aiMesh* mesh = meshSource->m_Scene->mMeshes[m];
			aiMaterial* material = meshSource->m_Scene->mMaterials[m];

			for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
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

				if (mesh->HasTangentsAndBitangents())
				{
					mVector.x = mesh->mTangents[i].x;
					mVector.y = mesh->mTangents[i].y;
					mVector.z = mesh->mTangents[i].z;
					vertex.Tangent = mVector;

					mVector.x = mesh->mBitangents[i].x;
					mVector.y = mesh->mBitangents[i].y;
					mVector.z = mesh->mBitangents[i].z;
					vertex.Binormal = mVector;
				}

				vertex.Color = glm::vec3{ 1.0f };

				if (mesh->HasTextureCoords(0))
				{
					glm::vec2 mTexCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
					vertex.TexCoord = mTexCoords;
				}

				meshSource->m_Vertices.push_back(vertex);
			}

			for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
			{
				aiFace face = mesh->mFaces[i];

				for (uint32_t j = 0; j < face.mNumIndices; ++j)
					meshSource->m_Indices.push_back(face.mIndices[j]);
			}
		}

		meshSource->m_VertexBuffer = std::make_shared<VulkanVertexBuffer>(meshSource->m_Vertices.data(), (uint32_t)(meshSource->m_Vertices.size() * sizeof(Vertex)));
		meshSource->m_IndexBuffer = std::make_shared<VulkanIndexBuffer>(meshSource->m_Indices.data(), (uint32_t)(meshSource->m_Indices.size() * 4));
	}

	void MeshImporter::InvalidateMaterials(std::shared_ptr<MeshSource> meshSource)
	{
		for (uint32_t i = 0; i < meshSource->m_Scene->mNumMaterials; ++i)
		{
			aiMaterial* material = meshSource->m_Scene->mMaterials[i];
		}
	}

	void MeshImporter::TraverseNodes(std::shared_ptr<MeshSource> meshSource, aiNode* aNode, uint32_t nodeIndex)
	{
		MeshNode& node = meshSource->m_Nodes[nodeIndex];
		node.Name = aNode->mName.C_Str();
		node.LocalTransform = Utils::Mat4FromAIMatrix4(aNode->mTransformation);

		for (uint32_t i = 0; i < aNode->mNumMeshes; ++i)
		{
			uint32_t submeshIndex = aNode->mMeshes[i];
			auto& submesh = meshSource->m_Submeshes[submeshIndex];
			submesh.NodeName = aNode->mName.C_Str();
			submesh.LocalTransform = node.LocalTransform;

			node.Submeshes.push_back(submeshIndex);
		}

		uint32_t parentNodeIndex = (uint32_t)meshSource->m_Nodes.size() - 1;
		node.Children.resize(aNode->mNumChildren);
		for (uint32_t i = 0; i < aNode->mNumChildren; ++i)
		{
			MeshNode& child = meshSource->m_Nodes.emplace_back();
			uint32_t childIndex = (uint32_t)meshSource->m_Nodes.size() - 1;
			child.Parent = parentNodeIndex;
			meshSource->m_Nodes[nodeIndex].Children[i] = childIndex;
			TraverseNodes(meshSource, aNode->mChildren[i], childIndex);
		}
	}

}
