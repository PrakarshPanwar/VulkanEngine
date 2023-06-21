#pragma once
#include "VulkanCore/Asset/MaterialAsset.h"

namespace VulkanCore {

	class MaterialEditor
	{
	public:
		MaterialEditor(std::shared_ptr<MaterialAsset> materialAsset);
		~MaterialEditor();

		void OnImGuiRender();
	private:
		std::shared_ptr<MaterialAsset> m_MaterialAsset;
		bool m_OpenEditorWindow = true;

		static std::unordered_map<uint64_t, std::shared_ptr<Material>> s_OpenedMaterials;
	};

}
