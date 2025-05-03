#pragma once
#include "VulkanCore/Core/UUID.h"

namespace VulkanCore {

	using AssetHandle = UUID;

	enum class AssetType : uint16_t
	{
		None = 0,
		Scene,
		Texture2D,
		TextureCube,
		Mesh,	   // For Runtime YAML Mesh File(class Mesh)
		MeshAsset, // For Native Mesh File(class MeshSource)
		Material
	};

	namespace Utils {

		std::string AssetTypeToString(AssetType type);
		AssetType AssetTypeFromString(const std::string& assetType);

	}

	class Asset
	{
	public:
		AssetHandle Handle = 0;

		virtual AssetType GetType() const = 0;
	};

	template<typename T>
	concept AssetConcept = requires
	{
		T::GetStaticType();
	} && std::derived_from<T, Asset>;

}
