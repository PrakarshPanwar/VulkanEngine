#pragma once
#include "Texture.h"

namespace VulkanCore {

	struct MSDFAtlasData;

	class Font
	{
	public:
		Font(const std::string& fontPath);
		~Font();

		const MSDFAtlasData* GetMSDFData() const { return m_MSDFData; }
		std::shared_ptr<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }
	private:
		MSDFAtlasData* m_MSDFData;
		std::shared_ptr<Texture2D> m_AtlasTexture;
	};

}
