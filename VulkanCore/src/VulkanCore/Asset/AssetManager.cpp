#include "vulkanpch.h"
#include "AssetManager.h"

#include "VulkanCore/Core/Core.h"

#include <yaml-cpp/yaml.h>

namespace VulkanCore {

	namespace Utils {

		std::string AssetTypeToString(AssetType type)
		{
			switch (type)
			{
			case AssetType::None:		 return "None";
			case AssetType::Scene:		 return "Scene";
			case AssetType::Texture2D:	 return "Texture 2D";
			case AssetType::TextureCube: return "Texture Cube";
			case AssetType::Mesh:		 return "Mesh";
			case AssetType::Material:	 return "Material";
			default:
				VK_CORE_ASSERT(false, "Asset Type not found!");
				return {};
			}
		}

	}

	std::shared_ptr<AssetManagerBase> AssetManager::s_AssetManager;

	void AssetManager::SetAssetManager(std::shared_ptr<AssetManagerBase> assetManager)
	{
		s_AssetManager = assetManager;
	}

	void AssetManager::LoadRegistryFromFile()
	{
		auto editorAssetManager = std::static_pointer_cast<EditorAssetManagerBase>(s_AssetManager);
		editorAssetManager->LoadRegistryFromFile();
	}

	void AssetManager::WriteRegistryToFile()
	{
		auto editorAssetManager = std::static_pointer_cast<EditorAssetManagerBase>(s_AssetManager);
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
			out << YAML::Key << "Type" << YAML::Value << (uint16_t)metadata.Type;
			out << YAML::EndMap; // End Metadata Map
			out << YAML::EndMap; // End Asset Map
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout("assets/AssetRegistry.vkr");
		fout << out.c_str();
	}

}
