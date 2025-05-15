#include "vulkanpch.h"
#include "SceneSerializer.h"

#include "VulkanCore/Core/Components.h"
#include "VulkanCore/Asset/AssetManager.h"
#include "VulkanCore/Asset/MaterialAsset.h"
#include "Entity.h"
#include "SceneRenderer.h"
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

	namespace Utils {

		static std::map<std::string, Rigidbody3DComponent::BodyType> s_BodyTypeFromStringMap = {
			{ "Static", Rigidbody3DComponent::BodyType::Static },
			{ "Dynamic", Rigidbody3DComponent::BodyType::Dynamic },
			{ "Kinematic", Rigidbody3DComponent::BodyType::Kinematic }
		};

		static std::string RigidBody3DBodyTypeToString(Rigidbody3DComponent::BodyType bodyType)
		{
			switch (bodyType)
			{
			case Rigidbody3DComponent::BodyType::Static:    return "Static";
			case Rigidbody3DComponent::BodyType::Dynamic:   return "Dynamic";
			case Rigidbody3DComponent::BodyType::Kinematic: return "Kinematic";
			default:
				VK_CORE_ASSERT(false, "Unknown body type");
				return {};
			}
		}

	}

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
			//out << YAML::Key << "Direction" << YAML::Value << slc.Direction;
			out << YAML::Key << "InnerCutoff" << YAML::Value << slc.InnerCutoff;
			out << YAML::Key << "OuterCutoff" << YAML::Value << slc.OuterCutoff;
			out << YAML::Key << "Falloff" << YAML::Value << slc.Falloff;
			out << YAML::Key << "Radius" << YAML::Value << slc.Radius;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<DirectionalLightComponent>())
		{
			out << YAML::Key << "DirectionalLightComponent";
			out << YAML::BeginMap;

			auto& drlc = entity.GetComponent<DirectionalLightComponent>();
			out << YAML::Key << "Direction" << YAML::Value << drlc.Direction;
			out << YAML::Key << "Color" << YAML::Value << drlc.Color;
			out << YAML::Key << "Falloff" << YAML::Value << drlc.Falloff;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<SkyLightComponent>())
		{
			out << YAML::Key << "SkyLightComponent";
			out << YAML::BeginMap;

			auto& slc = entity.GetComponent<SkyLightComponent>();
			out << YAML::Key << "TextureHandle" << YAML::Value << slc.TextureHandle;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<MeshComponent>())
		{
			out << YAML::Key << "MeshComponent";
			out << YAML::BeginMap;

			auto& mc = entity.GetComponent<MeshComponent>();
			out << YAML::Key << "MeshHandle" << YAML::Value << mc.MeshHandle;
			out << YAML::Key << "MaterialTable" << YAML::Value;

			out << YAML::BeginMap; // Begin Material Table
			for (auto& [materialIndex, materialAsset] : mc.MaterialTableHandle->GetMaterialMap())
			{
				if (materialAsset)
					out << YAML::Key << materialIndex << YAML::Value << materialAsset->Handle;
			}

			out << YAML::EndMap; // End Material Table

			out << YAML::EndMap;
		}

		if (entity.HasComponent<Rigidbody3DComponent>())
		{
			out << YAML::Key << "Rigidbody3DComponent";
			out << YAML::BeginMap; // Rigidbody3DComponent

			auto& rb3dComponent = entity.GetComponent<Rigidbody3DComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << Utils::RigidBody3DBodyTypeToString(rb3dComponent.Type);
			out << YAML::Key << "FixedRotation" << YAML::Value << rb3dComponent.FixedRotation;

			out << YAML::EndMap; // Rigidbody3DComponent
		}

		if (entity.HasComponent<BoxCollider3DComponent>())
		{
			out << YAML::Key << "BoxCollider3DComponent";
			out << YAML::BeginMap; // BoxCollider3DComponent

			auto& bc3dComponent = entity.GetComponent<BoxCollider3DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << bc3dComponent.Offset;
			out << YAML::Key << "Size" << YAML::Value << bc3dComponent.Size;
			out << YAML::Key << "Density" << YAML::Value << bc3dComponent.Density;
			out << YAML::Key << "Friction" << YAML::Value << bc3dComponent.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << bc3dComponent.Restitution;
			//out << YAML::Key << "RestitutionThreshold" << YAML::Value << bc3dComponent.RestitutionThreshold;

			out << YAML::EndMap; // BoxCollider3DComponent
		}

		if (entity.HasComponent<SphereColliderComponent>())
		{
			out << YAML::Key << "SphereColliderComponent";
			out << YAML::BeginMap; // SphereColliderComponent

			auto& sc3dComponent = entity.GetComponent<SphereColliderComponent>();
			out << YAML::Key << "Offset" << YAML::Value << sc3dComponent.Offset;
			out << YAML::Key << "Radius" << YAML::Value << sc3dComponent.Radius;
			out << YAML::Key << "Density" << YAML::Value << sc3dComponent.Density;
			out << YAML::Key << "Friction" << YAML::Value << sc3dComponent.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << sc3dComponent.Restitution;
			//out << YAML::Key << "RestitutionThreshold" << YAML::Value << cc3dComponent.RestitutionThreshold;

			out << YAML::EndMap; // SphereColliderComponent
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

		auto view = m_Scene->m_Registry.view<entt::entity>();
		for (auto entityID : view)
		{
			Entity entity = { entityID, m_Scene.get() };
			if (!entity)
				continue;

			SerializeEntity(out, entity);
		}

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
					auto& tc = deserializedEntity.AddComponent<TransformComponent>();

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
					//slc.Direction = spotLightComponent["Direction"].as<glm::vec3>();
					slc.InnerCutoff = spotLightComponent["InnerCutoff"].as<float>();
					slc.OuterCutoff = spotLightComponent["OuterCutoff"].as<float>();
					slc.Falloff = spotLightComponent["Falloff"].as<float>();
					slc.Radius = spotLightComponent["Radius"].as<float>();
				}

				auto directionalLightComponent = entity["DirectionalLightComponent"];
				if (directionalLightComponent)
				{
					auto& drlc = deserializedEntity.AddComponent<DirectionalLightComponent>();

					drlc.Direction = directionalLightComponent["Direction"].as<glm::vec3>();
					drlc.Color = directionalLightComponent["Color"].as<glm::vec4>();
					drlc.Falloff = directionalLightComponent["Falloff"].as<float>();
				}

				auto skyLightComponent = entity["SkyLightComponent"];
				if (skyLightComponent)
				{
					auto& slc = deserializedEntity.AddComponent<SkyLightComponent>();

					uint64_t textureHandle = skyLightComponent["TextureHandle"].as<uint64_t>();
					slc.TextureHandle = textureHandle;

					SceneRenderer::SetSkybox(slc.TextureHandle);
				}

				auto meshComponent = entity["MeshComponent"];
				if (meshComponent)
				{
					auto& mc = deserializedEntity.AddComponent<MeshComponent>();

					uint64_t meshHandle = meshComponent["MeshHandle"].as<uint64_t>();
					mc.MeshHandle = meshHandle;

					// Only contains Material handles
					auto materialTable = meshComponent["MaterialTable"].as<std::map<uint32_t, uint64_t>>();

					// Creating Material Table
					std::map<uint32_t, std::shared_ptr<MaterialAsset>> materialTableAssets{};
					for (auto& [materialIndex, materialHandle] : materialTable)
						materialTableAssets[materialIndex] = AssetManager::GetAsset<MaterialAsset>(materialHandle);

					// Add null material for submeshes that don't have a material
					std::shared_ptr<Mesh> mesh = AssetManager::GetAsset<Mesh>(mc.MeshHandle);
					for (auto& submesh : mesh->GetMeshSource()->GetSubmeshes())
					{
						if (!materialTableAssets.contains(submesh.MaterialIndex))
							materialTableAssets[submesh.MaterialIndex] = nullptr;
					}

					mc.MaterialTableHandle = std::make_shared<MaterialTable>(materialTableAssets);

					std::shared_ptr<MaterialAsset> materialAsset = materialTableAssets[0]; // Default Base Material
					std::shared_ptr<MeshSource> meshSource = mesh->GetMeshSource();
					meshSource->SetBaseMaterial(materialAsset);
				}

				auto rigidbody3DComponent = entity["Rigidbody3DComponent"];
				if (rigidbody3DComponent)
				{
					auto& rb3d = deserializedEntity.AddComponent<Rigidbody3DComponent>();
					rb3d.Type = Utils::s_BodyTypeFromStringMap[rigidbody3DComponent["BodyType"].as<std::string>()];
					rb3d.FixedRotation = rigidbody3DComponent["FixedRotation"].as<bool>();
				}

				auto boxCollider3DComponent = entity["BoxCollider3DComponent"];
				if (boxCollider3DComponent)
				{
					auto& bc3d = deserializedEntity.AddComponent<BoxCollider3DComponent>();
					bc3d.Offset = boxCollider3DComponent["Offset"].as<glm::vec3>();
					bc3d.Size = boxCollider3DComponent["Size"].as<glm::vec3>();
					bc3d.Density = boxCollider3DComponent["Density"].as<float>();
					bc3d.Friction = boxCollider3DComponent["Friction"].as<float>();
					bc3d.Restitution = boxCollider3DComponent["Restitution"].as<float>();
					//bc3d.RestitutionThreshold = boxCollider3DComponent["RestitutionThreshold"].as<float>();
				}

				auto sphereColliderComponent = entity["SphereColliderComponent"];
				if (sphereColliderComponent)
				{
					auto& sc3d = deserializedEntity.AddComponent<SphereColliderComponent>();
					sc3d.Offset = sphereColliderComponent["Offset"].as<glm::vec3>();
					sc3d.Radius = sphereColliderComponent["Radius"].as<float>();
					sc3d.Density = sphereColliderComponent["Density"].as<float>();
					sc3d.Friction = sphereColliderComponent["Friction"].as<float>();
					sc3d.Restitution = sphereColliderComponent["Restitution"].as<float>();
					//sc3d.RestitutionThreshold = sphereColliderComponent["RestitutionThreshold"].as<float>();
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
