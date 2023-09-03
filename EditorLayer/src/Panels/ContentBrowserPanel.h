#pragma once
#include "VulkanCore/Renderer/Texture.h"
#include "MaterialEditor.h"

namespace VulkanCore {

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();
		~ContentBrowserPanel();

		void SetThumbnailSize(float thumbnailSize) { m_ThumbnailSize = thumbnailSize; }
		void SetPadding(float padding) { m_Padding = padding; }

		void OnImGuiRender();
	private:
		void MeshImportDialog(bool open, const std::filesystem::path& path);
		void RemoveAssetDialog(bool open, const std::filesystem::path& path);
		void CreateMaterialDialog();
	private:
		struct TreeNode
		{
			std::filesystem::path FilePath;

			TreeNode* ParentNode = nullptr;
			std::list<TreeNode> ChildNodes;
		};

		void UpdateAssetTree(TreeNode* treeNode);
	private:
		std::filesystem::path m_CurrentDirectory;
		TreeNode* m_RootNode = nullptr;
		bool m_AssetMode = true;

		std::shared_ptr<Texture2D> m_DirectoryIcon, m_FileIcon;
		VkDescriptorSet m_DirectoryIconID, m_FileIconID;

		std::shared_ptr<MaterialEditor> m_MaterialEditor;

		float m_ThumbnailSize = 128.0f, m_Padding = 16.0f;
	};

}
