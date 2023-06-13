#pragma once
#include "MaterialAsset.h"

namespace VulkanCore {

	class MaterialAssetImporter
	{
	public:
		MaterialAssetImporter();
		~MaterialAssetImporter();

		std::string SerializeToYAML(MaterialAsset& materialAsset);
		bool DeserializeFromYAML(const std::string& filepath, MaterialAsset& materialAsset);
	private:
	};

}
