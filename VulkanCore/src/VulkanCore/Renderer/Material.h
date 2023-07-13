#pragma once
#include "VulkanCore/Renderer/Image.h"
#include "VulkanCore/Renderer/Texture.h"
#include "VulkanCore/Renderer/UniformBuffer.h"
#include "RenderCommandBuffer.h"
#include "ComputePipeline.h"
#include "Pipeline.h"

#include <glm/glm.hpp>

namespace VulkanCore {

	struct MaterialData
	{
		glm::vec4 Albedo{ 1.0f };
		float Emission = 1.0f;
		float Roughness = 1.0f;
		float Metallic = 1.0f;
		uint32_t UseNormalMap = 0;
	};

	class Scene;

	class Material
	{
	public:
		Material() = default;
		Material(const MaterialData& materialData);

		void SetAlbedo(const glm::vec4& albedo);
		void SetMetallic(float mettalic);
		void SetRoughness(float roughness);
		void SetMaterialData(MaterialData materialData);

		std::tuple<AssetHandle, AssetHandle, AssetHandle> GetMaterialHandles() const;

		virtual void SetDiffuseTexture(std::shared_ptr<Texture2D> texture);
		virtual void SetNormalTexture(std::shared_ptr<Texture2D> texture);
		virtual void SetARMTexture(std::shared_ptr<Texture2D> texture);

		virtual void SetImage(uint32_t binding, std::shared_ptr<Image2D> image) = 0;
		virtual void SetImage(uint32_t binding, std::shared_ptr<Image2D> image, uint32_t mipLevel) = 0;
		virtual void SetImages(uint32_t binding, const std::vector<std::shared_ptr<Image2D>>& images) = 0;
		virtual void SetTexture(uint32_t binding, std::shared_ptr<Texture2D> texture) = 0;
		virtual void SetTexture(uint32_t binding, std::shared_ptr<TextureCube> textureCube) = 0;
		virtual void SetTextures(uint32_t binding, const std::vector<std::shared_ptr<Texture2D>>& textures) = 0;
		virtual void SetBuffer(uint32_t binding, std::shared_ptr<UniformBuffer> uniformBuffer) = 0;
		virtual void SetBuffers(uint32_t binding, const std::vector<std::shared_ptr<UniformBuffer>>& uniformBuffers) = 0;

		virtual void RT_BindMaterial(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<Pipeline>& pipeline, uint32_t setIndex = 0) = 0;
		virtual void RT_BindMaterial(const std::shared_ptr<RenderCommandBuffer>& cmdBuffer, const std::shared_ptr<ComputePipeline>& pipeline, uint32_t setIndex = 0) = 0;

		virtual void UpdateMaterials() = 0;
		virtual void PrepareShaderMaterial() = 0;

		inline MaterialData& GetMaterialData() { return m_MaterialData; }

		static std::shared_ptr<Material> Create(const std::string& debugName);
	protected:
		MaterialData m_MaterialData{};

		std::shared_ptr<Texture2D> m_DiffuseTexture;
		std::shared_ptr<Texture2D> m_NormalTexture;
		std::shared_ptr<Texture2D> m_ARMTexture;
	};

}
