#include "vulkanpch.h"
#include "SceneImporter.h"
#include "AssetManager.h"
#include "VulkanCore/Scene/SceneSerializer.h"

namespace VulkanCore {

	std::shared_ptr<Scene> SceneImporter::ImportScene(AssetHandle handle, const AssetMetadata& metadata)
	{
		std::shared_ptr<Scene> scene = std::make_shared<Scene>();

		SceneSerializer serializer(scene);
		if (serializer.Deserialize(metadata.FilePath.string()))
			return scene;

		return nullptr;
	}

	void SceneImporter::SerializeScene(const AssetMetadata& metadata, std::shared_ptr<Asset> asset)
	{
		std::shared_ptr<Scene> scene = std::dynamic_pointer_cast<Scene>(asset);

		SceneSerializer serializer(scene);
		serializer.Serialize(metadata.FilePath.string());
	}

}
