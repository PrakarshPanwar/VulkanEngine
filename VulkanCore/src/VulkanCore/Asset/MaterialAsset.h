#pragma once
#include "VulkanCore/Renderer/Material.h"
#include "VulkanCore/Asset/Asset.h"

namespace VulkanCore {

	class MaterialAsset : public Asset
	{
	public:
		MaterialAsset(const std::string& filepath);
		~MaterialAsset();

		void SaveAsset();
		void NewAsset();

		AssetType GetType() const override { return AssetType::Material; }

		inline std::shared_ptr<Material> GetMaterial() const { return m_Material; }
	private:
		std::string m_FilePath;
		uint64_t m_MaterialAssetHandle = 0;

		std::shared_ptr<Material> m_Material;
	};

}
