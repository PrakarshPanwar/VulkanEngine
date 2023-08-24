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
		void SetTransparent(bool transparent);

		inline std::shared_ptr<Material> GetMaterial() const { return m_Material; }
		inline bool IsTransparent() const { return m_Transparent; }

		inline AssetType GetType() const override { return AssetType::Material; }
		static AssetType GetStaticType() { return AssetType::Material; }
	private:
		std::shared_ptr<Material> m_Material;
		bool m_Transparent = false;
	};

}
