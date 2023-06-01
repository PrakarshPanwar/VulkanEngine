#include "vulkanpch.h"
#include "Material.h"
#include "Renderer.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace VulkanCore {

	Material::Material(const MaterialData& materialData)
		: m_MaterialData(materialData)
	{
		m_DiffuseTexture = Renderer::GetWhiteTexture(ImageFormat::RGBA8_SRGB);
		m_NormalTexture = Renderer::GetWhiteTexture(ImageFormat::RGBA8_UNORM);
		m_ARMTexture = Renderer::GetWhiteTexture(ImageFormat::RGBA8_UNORM);
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

	void Material::SetMaterialData(MaterialData materialData)
	{
		m_MaterialData = materialData;
	}

	std::tuple<std::string, std::string, std::string> Material::GetMaterialPaths() const
	{
		return {};
	}

	void Material::SetDiffuseTexture(std::shared_ptr<Texture2D> texture)
	{
		m_DiffuseTexture = texture;
	}

	void Material::SetNormalTexture(std::shared_ptr<Texture2D> texture)
	{
		m_NormalTexture = texture;
	}

	void Material::SetARMTexture(std::shared_ptr<Texture2D> texture)
	{
		m_ARMTexture = texture;
	}

}
