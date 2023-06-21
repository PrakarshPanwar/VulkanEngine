#pragma once
#include "VulkanCore/Renderer/Material.h"
#include "VulkanCore/Asset/Asset.h"

namespace VulkanCore {

	class MaterialAsset : public Asset
	{
	public:
		MaterialAsset(std::shared_ptr<Material> material);
		~MaterialAsset();

		void Serialize();

		AssetType GetType() const override { return AssetType::Material; }
		inline std::shared_ptr<Material> GetMaterial() const { return m_Material; }
	private:
		std::shared_ptr<Material> m_Material;
	};

}
