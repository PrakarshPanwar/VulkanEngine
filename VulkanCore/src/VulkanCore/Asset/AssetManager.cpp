#include "vulkanpch.h"
#include "AssetManager.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Timer.h"

#include <yaml-cpp/yaml.h>

namespace VulkanCore {

	std::shared_ptr<AssetManagerBase> AssetManager::s_AssetManager;

	void AssetManager::SetAssetManagerBase(const std::shared_ptr<AssetManagerBase>& assetManager)
	{
		s_AssetManager = assetManager;
	}

	void AssetManager::Init(const std::shared_ptr<AssetManagerBase>& assetManager)
	{
		SetAssetManagerBase(assetManager);
		LoadRegistryFromFile();
	}

	void AssetManager::Shutdown()
	{
		auto editorAssetManager = AssetManager::GetEditorAssetManager();
		editorAssetManager->ClearMemoryOnlyAssets();
		editorAssetManager->ClearLoadedAssets();
		editorAssetManager->ClearAssetRegistry();
	}

	bool AssetManager::LoadRegistryFromFile()
	{
		std::unique_ptr<Timer> timer = std::make_unique<Timer>("Loading Asset Registry");
		auto editorAssetManager = GetEditorAssetManager();

		std::ifstream stream("assets/AssetRegistry.vkr");
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Asset Registry"])
			return false;

		auto assetRegistry = data["Asset Registry"];
		if (assetRegistry)
		{
			for (auto asset : assetRegistry)
			{
				AssetHandle assetHandle = asset["Handle"].as<uint64_t>();
				AssetMetadata assetMetadata{};

				auto metadata = asset["Metadata"];
				if (metadata)
				{
					assetMetadata.FilePath = metadata["Filepath"].as<std::string>();
					assetMetadata.Type = Utils::AssetTypeFromString(metadata["Type"].as<std::string>());
				}

				editorAssetManager->WriteToAssetRegistry(assetHandle, assetMetadata);
			}
		}

		return true;
	}

	void AssetManager::WriteRegistryToFile()
	{
		auto editorAssetManager = GetEditorAssetManager();
		const AssetRegistry& assetRegistry = editorAssetManager->GetAssetRegistry();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Asset Registry";
		out << YAML::Value << YAML::BeginSeq;
		for (auto&& [handle, metadata] : assetRegistry)
		{
			out << YAML::BeginMap; // Map for saving handle
			out << YAML::Key << "Handle" << YAML::Value << handle;
			out << YAML::Key << "Metadata";
			out << YAML::BeginMap; // Map for saving metadata
			out << YAML::Key << "Filepath" << YAML::Value << metadata.FilePath.string();
			out << YAML::Key << "Type" << YAML::Value << Utils::AssetTypeToString(metadata.Type);
			out << YAML::EndMap; // End Metadata Map
			out << YAML::EndMap; // End Asset Map
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout("assets/AssetRegistry.vkr");
		fout << out.c_str();
	}

}
