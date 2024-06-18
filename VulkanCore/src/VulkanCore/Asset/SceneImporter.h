#pragma once
#include "AssetMetadata.h"
#include "VulkanCore/Scene/Scene.h"

namespace VulkanCore {

	class SceneImporter
	{
	public:
		static std::shared_ptr<Scene> ImportScene(AssetHandle handle, const AssetMetadata& metadata);
		static void SerializeScene(const AssetMetadata& metadata, std::shared_ptr<Asset> asset);
	};

}
