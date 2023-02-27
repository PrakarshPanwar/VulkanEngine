#include "vulkanpch.h"
#include "Material.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace VulkanCore {

	Material::Material(const MaterialData& materialData)
		: m_MaterialData(materialData)
	{
	}

	void Material::SetAlbedo(const glm::vec4& albedo)
	{
		m_MaterialData.Albedo = albedo;
	}

	void Material::SetMetallic(float mettalic)
	{
		m_MaterialData.Metallic = mettalic;
	}

	void Material::SetRoughness(float roughness)
	{
		m_MaterialData.Roughness = roughness;
	}

}
