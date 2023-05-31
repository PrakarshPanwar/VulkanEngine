#include "vulkanpch.h"
#include "MaterialAssetSerializer.h"

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

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	MaterialAssetSerializer::MaterialAssetSerializer()
	{
	}

	MaterialAssetSerializer::~MaterialAssetSerializer()
	{
	}

	std::string MaterialAssetSerializer::SerializeToYAML(MaterialAsset& materialAsset)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Material" << YAML::Value;
		{
			out << YAML::BeginMap;

			auto material = materialAsset.GetMaterial();
			MaterialData& materialData = material->GetMaterialData();
			out << YAML::Key << "Albedo" << YAML::Value << materialData.Albedo;
			out << YAML::Key << "Metallic" << YAML::Value << materialData.Metallic;
			out << YAML::Key << "Roughness" << YAML::Value << materialData.Roughness;
			out << YAML::Key << "UseNormalMap" << YAML::Value << materialData.UseNormalMap;

			// Setting Materials Path
			auto [diffusePath, normalPath, armPath] = material->GetMaterialPaths();
			out << YAML::Key << "AlbedoTexture" << YAML::Value << diffusePath;
			out << YAML::Key << "NormalTexture" << YAML::Value << normalPath;
			out << YAML::Key << "ARMTexture" << YAML::Value << armPath;

			out << YAML::EndMap;
		}

		out << YAML::EndMap; // End Material Map

		return {};
	}

	bool MaterialAssetSerializer::DeserializeFromYAML(const std::string& filepath, MaterialAsset& materialAsset)
	{
		std::ifstream stream(filepath);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Material"])
			return false;

		auto materialData = data["Material"];

		std::shared_ptr<Material> material = materialAsset.GetMaterial();
		glm::vec4 albedoColor = materialData["Albedo"].as<glm::vec4>();
		float metallic = materialData["Metallic"].as<float>();
		float roughness = materialData["Roughness"].as<float>();
		uint32_t useNormalMap = materialData["UseNormalMap"].as<uint32_t>();
		material->SetMaterialData({ albedoColor, roughness, metallic, useNormalMap });

		std::string albedoPath = materialData["AlbedoTexture"].as<std::string>();
		std::string normalPath = materialData["NormalTexture"].as<std::string>();
		std::string armPath = materialData["ARMTexture"].as<std::string>();

		auto vulkanMaterial = std::dynamic_pointer_cast<VulkanMaterial>(material);
		vulkanMaterial->UpdateMaterials(albedoPath, normalPath, armPath);
	}

}
