#pragma once
#include "VulkanCore/Renderer/Material.h"

namespace VulkanCore {

	class MaterialEditor
	{
	public:
		MaterialEditor(std::shared_ptr<Material> material);
		~MaterialEditor();

		void OnImGuiRender();
	private:
		std::shared_ptr<Material> m_Material;
		bool m_OpenEditorWindow = true;

		static std::unordered_map<uint64_t, std::shared_ptr<Material>> s_OpenedMaterials;
	};

}
