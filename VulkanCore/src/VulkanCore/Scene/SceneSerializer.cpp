#include "vulkanpch.h"
#include "SceneSerializer.h"

#include "VulkanCore/Core/Components.h"
#include "VulkanCore/Asset/AssetManager.h"
#include "VulkanCore/Asset/MaterialAsset.h"
#include "Entity.h"
#include "Platform/Vulkan/VulkanMaterial.h"

#include <yaml-cpp/yaml.h>

namespace YAML {

	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

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

	static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	static void SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap;

			auto& tag = entity.GetComponent<TagComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap;

			auto& tc = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Translation" << YAML::Value << tc.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << tc.Scale;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<PointLightComponent>())
		{
			out << YAML::Key << "PointLightComponent";
			out << YAML::BeginMap;

			auto& plc = entity.GetComponent<PointLightComponent>();
			out << YAML::Key << "Color" << YAML::Value << plc.Color;
			out << YAML::Key << "Falloff" << YAML::Value << plc.Falloff;
			out << YAML::Key << "Radius" << YAML::Value << plc.Radius;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<SpotLightComponent>())
		{
			out << YAML::Key << "SpotLightComponent";
			out << YAML::BeginMap;

			auto& slc = entity.GetComponent<SpotLightComponent>();
			out << YAML::Key << "Color" << YAML::Value << slc.Color;
			out << YAML::Key << "Direction" << YAML::Value << slc.Direction;
			out << YAML::Key << "InnerCutoff" << YAML::Value << slc.InnerCutoff;
			out << YAML::Key << "OuterCutoff" << YAML::Value << slc.OuterCutoff;
			out << YAML::Key << "Falloff" << YAML::Value << slc.Falloff;
			out << YAML::Key << "Radius" << YAML::Value << slc.Radius;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<MeshComponent>())
		{
			out << YAML::Key << "MeshComponent";
			out << YAML::BeginMap;

			auto& mc = entity.GetComponent<MeshComponent>();
			out << YAML::Key << "MeshHandle" << YAML::Value << mc.MeshHandle;
			out << YAML::Key << "MaterialHandle" << YAML::Value << mc.MaterialTableHandle;

			out << YAML::EndMap;
		}

		out << YAML::EndMap; // Entity
	}

	SceneSerializer::SceneSerializer(std::shared_ptr<Scene> scene)
		: m_Scene(scene)
	{
	}

	void SceneSerializer::Serialize(const std::string& filepath)
	{
		std::filesystem::path scenePath = filepath;

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << scenePath.stem().string();
		out << YAML::Key << "Entities";
		out << YAML::Value << YAML::BeginSeq;
		m_Scene->m_Registry.each([&](auto entityID)
		{
			Entity entity = { entityID, m_Scene.get() };
			if (!entity)
				return;

			SerializeEntity(out, entity);
		});
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	void SceneSerializer::SerializeRuntime(const std::string& filepath)
	{

	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		std::ifstream stream(filepath);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();
		VK_CORE_TRACE("Deserializing Scene: '{}'", sceneName);

		auto entities = data["Entities"];

		if (entities)
		{
			for (auto entity : entities)
			{
				uint64_t uuid = entity["Entity"].as<uint64_t>();

				std::string name;
				auto tagComponent = entity["TagComponent"];
				if (tagComponent)
					name = tagComponent["Tag"].as<std::string>();

				VK_CORE_TRACE("Deserializing entity with ID = {0}, name = {1}", uuid, name);

				Entity deserializedEntity = m_Scene->CreateEntityWithUUID(uuid, name);

				auto transformComponent = entity["TransformComponent"];
				if (transformComponent)
				{
					auto& tc = deserializedEntity.GetComponent<TransformComponent>();
					tc.Translation = transformComponent["Translation"].as<glm::vec3>();
					tc.Rotation = transformComponent["Rotation"].as<glm::vec3>();
					tc.Scale = transformComponent["Scale"].as<glm::vec3>();
				}

				auto pointLightComponent = entity["PointLightComponent"];
				if (pointLightComponent)
				{
					auto& plc = deserializedEntity.AddComponent<PointLightComponent>();

					plc.Color = pointLightComponent["Color"].as<glm::vec4>();
					plc.Falloff = pointLightComponent["Falloff"].as<float>();
					plc.Radius = pointLightComponent["Radius"].as<float>();
				}

				auto spotLightComponent = entity["SpotLightComponent"];
				if (spotLightComponent)
				{
					auto& slc = deserializedEntity.AddComponent<SpotLightComponent>();

					slc.Color = spotLightComponent["Color"].as<glm::vec4>();
					slc.Direction = spotLightComponent["Direction"].as<glm::vec3>();
					slc.InnerCutoff = spotLightComponent["InnerCutoff"].as<float>();
					slc.OuterCutoff = spotLightComponent["OuterCutoff"].as<float>();
					slc.Falloff = spotLightComponent["Falloff"].as<float>();
					slc.Radius = spotLightComponent["Radius"].as<float>();
				}

				auto meshComponent = entity["MeshComponent"];
				if (meshComponent)
				{
					auto& mc = deserializedEntity.AddComponent<MeshComponent>();

					uint64_t meshHandle = meshComponent["MeshHandle"].as<uint64_t>();
					mc.MeshHandle = meshHandle;

					uint64_t materialHandle = meshComponent["MaterialHandle"].as<uint64_t>();
					mc.MaterialTableHandle = materialHandle;

					std::shared_ptr<Mesh> mesh = AssetManager::GetAsset<Mesh>(mc.MeshHandle);
					std::shared_ptr<MaterialAsset> materialAsset = AssetManager::GetAsset<MaterialAsset>(mc.MaterialTableHandle);
					std::shared_ptr<MeshSource> meshSource = mesh->GetMeshSource();
					meshSource->SetMaterial(materialAsset->GetMaterial());
				}
			}
		}

		return true;
	}

	bool SceneSerializer::DeserializeRuntime(const std::string& filepath)
	{
		return false;
	}

}
