#pragma once
#include "VulkanCore/Renderer/Camera.h"
#include "VulkanCore/Mesh/Mesh.h"
#include "VulkanCore/Asset/MaterialAsset.h"
#include "VulkanCore/Core/UUID.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace VulkanCore {

	struct TransformComponent
	{
		glm::vec3 Translation{ 0.0f }, Rotation{ 0.0f }, Scale{ 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation, const glm::vec3& scale)
			: Translation(translation), Scale(scale) {}

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

			return glm::translate(glm::mat4(1.0f), Translation)
				* rotation
				* glm::scale(glm::mat4(1.0f), Scale);
		}

		glm::mat3 GetNormalMatrix() const
		{
			const float c3 = glm::cos(Rotation.z);
			const float s3 = glm::sin(Rotation.z);
			const float c2 = glm::cos(Rotation.x);
			const float s2 = glm::sin(Rotation.x);
			const float c1 = glm::cos(Rotation.y);
			const float s1 = glm::sin(Rotation.y);
			const glm::vec3 inverseScale = 1.0f / Scale;

			return glm::mat3(
				{
					inverseScale.x * (c1 * c3 + s1 * s2 * s3),
					inverseScale.x * (c2 * s3),
					inverseScale.x * (c1 * s2 * s3 - c3 * s1)
				},
				
				{
					inverseScale.y * (c3 * s1 * s2 - c1 * s3),
					inverseScale.y * (c2 * c3),
					inverseScale.y * (c1 * c3 * s2 + s1 * s3)
				},
				
				{
					inverseScale.z * (c2 * s1),
					inverseScale.z * (-s2),
					inverseScale.z * (c1 * c2)
				});
		}
	};

	struct IDComponent
	{
		UUID ID;

		IDComponent() = default;
		IDComponent(const UUID& id)
			: ID(id) {}

		IDComponent(const IDComponent&) = default;
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {}
	};

	class Mesh;
	class MaterialAsset;

	struct MeshComponent
	{
		AssetHandle MeshHandle = 0;
		std::shared_ptr<MaterialTable> MaterialTableHandle;

		MeshComponent() = default;
		MeshComponent(const MeshComponent&) = default;
	};

	struct TransformData
	{
		glm::vec4 MRow[3];
	};

	struct SelectTransformData
	{
		glm::vec4 MRow[3];
		int EntityID = -1;
	};

	struct alignas(16) PointLightComponent
	{
		glm::vec4 Position{ 0.0f };
		glm::vec4 Color{ 0.0f };
		float Radius = 0.1f;
		float Falloff = 1.0f;

		PointLightComponent() = default;
		PointLightComponent(const glm::vec4& color)
			: Color(color) {}

		PointLightComponent(const glm::vec4& position, const glm::vec4& color, float radius, float falloff)
			: Position(position), Color(color), Radius(radius), Falloff(falloff) {}
	};

	struct alignas(8) SpotLightComponent
	{
		glm::vec4 Position{ 0.0f };
		glm::vec4 Color{ 1.0f };
		glm::vec3 Direction{ 1.0f };
		float InnerCutoff = 0.174f;
		float OuterCutoff = 0.261f; // NOTE: OuterCutoff > InnerCutoff
		float Radius = 0.1f;
		float Falloff = 1.0f;

		SpotLightComponent() = default;
		SpotLightComponent(const glm::vec4& color)
			: Color(color) {}

		SpotLightComponent(const glm::vec4& position, const glm::vec4& color, const glm::vec3& direction, float innerCutoff, float outerCutoff, float radius, float falloff)
			: Position(position), Color(color), Direction(direction),
			InnerCutoff(innerCutoff), OuterCutoff(outerCutoff),
			Radius(radius), Falloff(falloff) {}
	};

	struct DirectionalLightComponent
	{
		glm::vec4 Color{ 1.0f };
		glm::vec3 Direction{ 1.0f };
		float Falloff = 1.0f;
	};

	// TODO: More variables will be added
	struct SkyLightComponent
	{
		AssetHandle TextureHandle = 0;
	};

	struct Rigidbody3DComponent // Entity ID will be Body ID
	{
		enum class BodyType { Static = 0, Dynamic, Kinematic };
		BodyType Type = BodyType::Static;
		bool FixedRotation = false;

		Rigidbody3DComponent() = default;
		Rigidbody3DComponent(const Rigidbody3DComponent&) = default;
	};

	struct BoxCollider3DComponent
	{
		glm::vec3 Offset{ 0.0f };
		glm::vec3 Size{ 0.5f };

		// TODO: Move into Physics Material in the future maybe
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		//float RestitutionThreshold = 0.5f;

		BoxCollider3DComponent() = default;
		BoxCollider3DComponent(const BoxCollider3DComponent&) = default;
	};

	struct SphereColliderComponent
	{
		glm::vec3 Offset{ 0.0f };
		float Radius = 0.5f;

		// TODO: Move into Physics Material in the future maybe
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		//float RestitutionThreshold = 0.5f;

		SphereColliderComponent() = default;
		SphereColliderComponent(const SphereColliderComponent&) = default;
	};

	struct CapsuleColliderComponent
	{
		float HalfHeight = 1.0f;
		float Radius = 0.5f;

		// TODO: Move into Physics Material in the future maybe
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		//float RestitutionThreshold = 0.5f;

		CapsuleColliderComponent() = default;
		CapsuleColliderComponent(const CapsuleColliderComponent&) = default;
	};

	struct MeshColliderComponent // Convex Hull Body
	{
		// TODO: Move into Physics Material in the future maybe
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		//float RestitutionThreshold = 0.5f;

		MeshColliderComponent() = default;
		MeshColliderComponent(const MeshColliderComponent&) = default;
	};

	struct UBCamera
	{
		glm::mat4 Projection{ 1.0f };
		glm::mat4 View{ 1.0f };
		glm::mat4 InverseView{ 1.0f };
		glm::vec2 DepthUnpackConsts{ 0.0f };
	};

	struct UBPointLights
	{
		int LightCount;
		glm::vec3 Padding{};
		PointLightComponent PointLights[10];
	};

	struct UBSpotLights
	{
		int LightCount;
		glm::vec3 Padding{};
		SpotLightComponent SpotLights[10];
	};

	struct UBDirectionalLights
	{
		int LightCount;
		glm::vec3 Padding{};
		DirectionalLightComponent DirectionalLights[2];
	};

	struct LightSelectData
	{
		glm::vec4 Position{};
		int EntityID = -1;
	};

	template<typename... Component>
	struct ComponentGroup
	{
	};

	template<typename T, typename... Component>
	struct IsComponent
	{
		static constexpr bool cvalue = (std::same_as<T, Component> || ...);
	};

	using AllComponents =
		ComponentGroup<TransformComponent, MeshComponent, PointLightComponent, SpotLightComponent, DirectionalLightComponent, SkyLightComponent,
			Rigidbody3DComponent, BoxCollider3DComponent, SphereColliderComponent, CapsuleColliderComponent, MeshColliderComponent>;

	template<typename... ComponentType>
	concept IsComponentType = (IsComponent<ComponentType, TransformComponent, MeshComponent, PointLightComponent, SpotLightComponent, DirectionalLightComponent, SkyLightComponent,
		Rigidbody3DComponent, BoxCollider3DComponent, SphereColliderComponent, CapsuleColliderComponent, MeshColliderComponent>::cvalue || ...);

}
