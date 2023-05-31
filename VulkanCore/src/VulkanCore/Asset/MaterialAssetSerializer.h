#pragma once
#include "MaterialAsset.h"

namespace VulkanCore {

	class MaterialAssetSerializer
	{
	public:
		MaterialAssetSerializer();
		~MaterialAssetSerializer();

		std::string SerializeToYAML(MaterialAsset& materialAsset);
		bool DeserializeFromYAML(const std::string& filepath, MaterialAsset& materialAsset);
	private:
	};

}
