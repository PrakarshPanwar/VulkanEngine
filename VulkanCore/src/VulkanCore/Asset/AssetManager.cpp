#include "vulkanpch.h"
#include "AssetManager.h"

#include <yaml-cpp/yaml.h>

namespace VulkanCore {

	std::shared_ptr<AssetManagerBase> AssetManager::s_AssetManager;

	void AssetManager::SetAssetManager(std::shared_ptr<AssetManagerBase> assetManager)
	{
		s_AssetManager = assetManager;
	}

	void AssetManager::WriteRegistryToFile()
	{
		auto editorAssetManager = std::static_pointer_cast<EditorAssetManagerBase>(s_AssetManager);
		const AssetRegistry& assetRegistry = editorAssetManager->GetAssetRegistry();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Asset Registry";

		for (auto&& [handle, metadata] : assetRegistry)
		{
			out << YAML::BeginMap;
			out << YAML::Key << handle << YAML::Value << metadata.FilePath.string();
			out << YAML::EndMap;
		}

		out << YAML::Value;

		std::ofstream fout("assets/AssetRegistry.vkr");
		fout << out.c_str();
	}

}
