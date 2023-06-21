#include "vulkanpch.h"
#include "MaterialAssetImporter.h"
#include "AssetManager.h"

#include "Platform/Vulkan/VulkanMaterial.h"

#include <yaml-cpp/yaml.h>

namespace YAML {

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

}

namespace VulkanCore {

	static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	std::shared_ptr<MaterialAsset> MaterialAssetImporter::ImportMaterialAsset(AssetHandle handle, const AssetMetadata& metadata)
	{
		std::shared_ptr<Asset> materialAsset = nullptr;

		bool loaded = DeserializeFromYAML(metadata, materialAsset);
		if (loaded)
			return std::static_pointer_cast<MaterialAsset>(materialAsset);

		return nullptr;
	}

	void MaterialAssetImporter::SerializeToYAML(const AssetMetadata& metadata, std::shared_ptr<Asset> asset)
	{
		std::string filepath = metadata.FilePath.string();
		std::shared_ptr<MaterialAsset> materialAsset = std::dynamic_pointer_cast<MaterialAsset>(asset);
		auto material = materialAsset->GetMaterial();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Material" << YAML::Value;
		{
			out << YAML::BeginMap;

			MaterialData& materialData = material->GetMaterialData();
			out << YAML::Key << "Albedo" << YAML::Value << materialData.Albedo;
			out << YAML::Key << "Metallic" << YAML::Value << materialData.Metallic;
			out << YAML::Key << "Roughness" << YAML::Value << materialData.Roughness;
			out << YAML::Key << "UseNormalMap" << YAML::Value << materialData.UseNormalMap;

			// Setting Materials Path
			auto [diffuseHandle, normalHandle, armHandle] = material->GetMaterialHandles();
			out << YAML::Key << "AlbedoHandle" << YAML::Value << diffuseHandle;
			out << YAML::Key << "NormalHandle" << YAML::Value << normalHandle;
			out << YAML::Key << "ARMHandle" << YAML::Value << armHandle;

			out << YAML::EndMap;
		}

		out << YAML::EndMap; // End Material Map

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

 	bool MaterialAssetImporter::DeserializeFromYAML(const AssetMetadata& metadata, std::shared_ptr<Asset>& asset)
 	{
		std::string filepath = metadata.FilePath.string();
 		std::ifstream stream(filepath);
 		std::stringstream strStream;
 		strStream << stream.rdbuf();
 
 		YAML::Node data = YAML::Load(strStream.str());
 		if (!data["Material"])
 			return false;
 
 		YAML::Node materialNode = data["Material"];
 
		std::string filenameStr = metadata.FilePath.stem().string();
		std::shared_ptr<Material> material = std::make_shared<VulkanMaterial>(filenameStr);
		std::shared_ptr<MaterialAsset> materialAsset = std::make_shared<MaterialAsset>(material);

 		glm::vec4 albedoColor = materialNode["Albedo"].as<glm::vec4>();
 		float metallic = materialNode["Metallic"].as<float>();
 		float roughness = materialNode["Roughness"].as<float>();
 		uint32_t useNormalMap = materialNode["UseNormalMap"].as<uint32_t>();
 		material->SetMaterialData({ albedoColor, roughness, metallic, useNormalMap });
 
 		AssetHandle albedoHandle = materialNode["AlbedoHandle"].as<uint64_t>();
 		AssetHandle normalHandle = materialNode["NormalHandle"].as<uint64_t>();
 		AssetHandle armHandle = materialNode["ARMHandle"].as<uint64_t>();
 
 		// Set Textures
		bool albedo = AssetManager::GetAssetManager()->IsAssetHandleValid(albedoHandle);
		bool normal = AssetManager::GetAssetManager()->IsAssetHandleValid(normalHandle);
		bool arm = AssetManager::GetAssetManager()->IsAssetHandleValid(armHandle);

		auto diffuseTexture = albedo ? AssetManager::GetAsset<Texture2D>(albedoHandle) : Renderer::GetWhiteTexture();
		auto normalTexture = normal ? AssetManager::GetAsset<VulkanTexture>(normalHandle) : Renderer::GetWhiteTexture(ImageFormat::RGBA8_UNORM);
		auto armTexture = arm ? AssetManager::GetAsset<VulkanTexture>(armHandle) : Renderer::GetWhiteTexture(ImageFormat::RGBA8_UNORM);

		normalTexture->Reload(ImageFormat::RGBA8_UNORM);
		armTexture->Reload(ImageFormat::RGBA8_UNORM);

 		material->SetDiffuseTexture(diffuseTexture);
 		material->SetNormalTexture(normalTexture);
 		material->SetARMTexture(armTexture);
 
 		auto vulkanMaterial = std::dynamic_pointer_cast<VulkanMaterial>(material);
 		vulkanMaterial->UpdateMaterials();

		asset = materialAsset;
		return true;
 	}

}
