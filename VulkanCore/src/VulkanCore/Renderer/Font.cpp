#include "vulkanpch.h"
#include "Font.h"

#include <msdf-atlas-gen/msdf-atlas-gen.h>

namespace VulkanCore {

	struct MSDFAtlasData
	{
		std::vector<msdf_atlas::GlyphGeometry> Glyphs{};
		msdf_atlas::FontGeometry* FontGeometry = nullptr;

		MSDFAtlasData()
		{
			// Initialize Font Geometry with Glyphs
			FontGeometry = new msdf_atlas::FontGeometry(&Glyphs);
		}

		~MSDFAtlasData()
		{
			delete FontGeometry;
		}
	};

	template<typename T, typename S, int N, msdf_atlas::GeneratorFunction<S, N> GEN_FN>
	static std::shared_ptr<Texture2D> CreateAtlas(const MSDFAtlasData* atlasData, glm::ivec2 atlasDims)
	{
		// This class generates atlas bitmap
		msdf_atlas::ImmediateAtlasGenerator<S, N, GEN_FN, msdf_atlas::BitmapAtlasStorage<T, N>> bitmapGenerator(atlasDims.x, atlasDims.y);

		// Generator attributes are used to modify generator default settings
		msdf_atlas::GeneratorAttributes genAttribs{};
		bitmapGenerator.setAttributes(genAttribs);
		bitmapGenerator.setThreadCount(4);

		// Glyph generation
		bitmapGenerator.generate(atlasData->Glyphs.data(), atlasData->Glyphs.size());

		// Bitmap data
		msdfgen::BitmapConstRef<T, N> bitmap = bitmapGenerator.atlasStorage();

		// Create atlas texture
		TextureSpecification atlasSpec{};
		atlasSpec.Width = bitmap.width;
		atlasSpec.Height = bitmap.height;
		atlasSpec.Format = ImageFormat::RGBA8_UNORM;
		atlasSpec.GenerateMips = false;

		// Copy bitmap pixels
		T* bitmapData = new uint8_t[bitmap.width * bitmap.height * N * sizeof(T)];
		memcpy(bitmapData, bitmap.pixels, bitmap.width * bitmap.height * N * sizeof(T));

		return Texture2D::Create(bitmapData, atlasSpec);
	}

	Font::Font(const std::string &fontPath)
	{
		// Initialize instance of Freetype library
		msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();

		// Load Font file
		msdfgen::FontHandle* font = msdfgen::loadFont(ft, fontPath.c_str());

		// Initialize Atlas Data and load charset
		m_MSDFData = new MSDFAtlasData();
		m_MSDFData->FontGeometry->loadCharset(font, 1.0f, msdf_atlas::Charset::ASCII);

		// Apply MSDF Edge Coloring
		const double maxCornerAngle = 3.0;
		for (msdf_atlas::GlyphGeometry& glyph : m_MSDFData->Glyphs)
			glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);

		// Class to compute atlas dimensions
		msdf_atlas::TightAtlasPacker atlasPacker{};
		atlasPacker.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
		atlasPacker.setMinimumScale(24.0f);
		atlasPacker.setPixelRange(2.0f);
		atlasPacker.setMiterLimit(1.0f);

		// Compute atlas layout
		atlasPacker.pack(m_MSDFData->Glyphs.data(), m_MSDFData->Glyphs.size());

		// Get atlas dimensions
		int width = 0, height = 0;
		atlasPacker.getDimensions(width, height);

		// Create atlas and store bitmap in 2D Texture(encoded with alpha channel)
		m_AtlasTexture = CreateAtlas<msdf_atlas::byte, float, 4, msdf_atlas::mtsdfGenerator>(m_MSDFData, { width, height });

		// Cleanup
		msdfgen::destroyFont(font);
		msdfgen::deinitializeFreetype(ft);
	}

	Font::~Font()
	{
		delete m_MSDFData;
	}

}
