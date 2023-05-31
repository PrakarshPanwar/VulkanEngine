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
		Mesh,
		Material
	};

	class Asset
	{
	public:
		AssetHandle Handle;

		virtual AssetType GetType() const = 0;
	};
}
