#pragma once
#include "VulkanCore/Renderer/Camera.h"

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

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {}
	};

	class Mesh;

	struct MeshComponent
	{
		std::shared_ptr<Mesh> Mesh;

		MeshComponent() = default;
		MeshComponent(const MeshComponent&) = default;
		MeshComponent(std::shared_ptr<Mesh> mesh)
			: Mesh(mesh) {}
	};

	struct PointLight
	{
		glm::vec4 Position{};
		glm::vec4 Color{};

		PointLight() = default;
		PointLight(const glm::vec4& position, const glm::vec4& color)
			: Position(position), Color(color) {}
	};

	struct PointLightComponent
	{
		std::shared_ptr<PointLight> PointLightInstance;

		PointLightComponent() = default;
		PointLightComponent(std::shared_ptr<PointLight> pointLight)
			: PointLightInstance(pointLight) {}
	};

	struct UBCamera
	{
		glm::mat4 Projection{ 1.0f };
		glm::mat4 View{ 1.0f };
		glm::mat4 InverseView{ 1.0f };
	};

	struct UBPointLights
	{
		glm::vec4 AmbientLightColor{ 1.0f, 1.0f, 1.0f, 0.02f };
		PointLight PointLights[10];
		int NumLights;
	};

	struct PCModelData
	{
		glm::mat4 ModelMatrix{ 1.f };
		glm::mat4 NormalMatrix{};
	};

	struct PCPointLight
	{
		glm::vec4 Position{};
		glm::vec4 Color{};
		float Radius;
	};

}