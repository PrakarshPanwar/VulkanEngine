#pragma once
#include "VulkanDevice.h"
#include "VulkanCore/Renderer/Texture.h"

namespace VulkanCore {

	class VulkanTexture : public Texture2D
	{
	public:
		VulkanTexture() = default;
		VulkanTexture(void* data, TextureSpecification spec = {});
		VulkanTexture(uint32_t width, uint32_t height, ImageFormat format);
		~VulkanTexture();

		const TextureSpecification& GetSpecification() const override { return m_Specification; }
		bool IsLoaded() const override { return m_IsLoaded; }

		const VulkanImageInfo& GetVulkanImageInfo() const { return m_Info; }
		const VkDescriptorImageInfo& GetDescriptorImageInfo() const { return m_Image->GetDescriptorImageInfo(); }

		void Reload(ImageFormat format);
	private:
		void Invalidate();
		void Release();
		void GenerateMipMaps();
	private:
		TextureSpecification m_Specification;

		std::shared_ptr<VulkanImage> m_Image;
		VulkanImageInfo m_Info;

		uint8_t* m_LocalStorage = nullptr;
		bool m_IsLoaded = false;
	};

	class VulkanTextureCube : public TextureCube
	{
	public:
		VulkanTextureCube(void* data, TextureSpecification spec = {});
		VulkanTextureCube(uint32_t width, uint32_t height, ImageFormat format = ImageFormat::RGBA32F);
		~VulkanTextureCube();

		const TextureSpecification& GetSpecification() const override { return m_Specification; }
		bool IsLoaded() const override { return m_IsLoaded; }

		void Invalidate();
		void GenerateMipMaps(bool readonly);
		VkImageView CreateImageViewSingleMip(uint32_t mip);
		VkImageView CreateImageViewPerLayer(uint32_t layer);
		const VkDescriptorImageInfo& GetDescriptorImageInfo() const { return m_DescriptorImageInfo; }
	private:
		void Release();
	private:
		std::vector<VkImageView> m_MipReferences;

		TextureSpecification m_Specification;
		VulkanImageInfo m_Info;
		VkDescriptorImageInfo m_DescriptorImageInfo;

		uint8_t* m_LocalStorage = nullptr;
		bool m_IsLoaded = false;
	};

}