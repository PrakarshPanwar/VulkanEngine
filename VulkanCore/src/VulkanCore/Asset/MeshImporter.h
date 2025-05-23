#pragma once
#include "AssetMetadata.h"
#include "VulkanCore/Mesh/Mesh.h"

namespace VulkanCore {

	class MeshImporter
	{
	public:
		static std::shared_ptr<MeshSource> ImportAssimpMesh(AssetHandle handle, const AssetMetadata& metadata);
		static std::shared_ptr<Mesh> ImportMesh(AssetHandle handle, const AssetMetadata& metadata);

		static void SerializeMesh(const AssetMetadata& metadata, std::shared_ptr<Asset> asset);
		static bool DeserializeMesh(const AssetMetadata& metadata, std::shared_ptr<Asset>& asset);
	private:
		static void InvalidateMesh(std::shared_ptr<MeshSource> meshSource);
		static void InvalidateMaterials(std::shared_ptr<MeshSource> meshSource);
		static void TraverseNodes(std::shared_ptr<MeshSource> meshSource, aiNode* aNode, uint32_t nodeIndex);
	};

}
