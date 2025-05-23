#include <vector>
#include <memory>
#include <filesystem>
#include <map>

#include "ContentBrowserPanel.h"
#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Timer.h"
#include "VulkanCore/Core/ImGuiLayer.h"
#include "VulkanCore/Asset/AssetManager.h"
#include "VulkanCore/Asset/MaterialAsset.h"
#include "VulkanCore/Asset/TextureImporter.h"
#include "VulkanCore/Mesh/Mesh.h"

#include <imgui.h>

namespace VulkanCore {

	namespace Utils {

		static std::map<std::filesystem::path, AssetType> s_AssetExtensionMap = {
			{ ".png", AssetType::Texture2D },
			{ ".jpg", AssetType::Texture2D },
			{ ".hdr", AssetType::Texture2D },
			{ ".fbx", AssetType::MeshAsset },
			{ ".gltf", AssetType::MeshAsset },
			{ ".obj", AssetType::MeshAsset },
			{ ".vkmesh", AssetType::Mesh },
			{ ".vkmat", AssetType::Material }
		};

		static bool IsAssetValid(const std::filesystem::path& filePath)
		{
			auto& assetRegistry = AssetManager::GetEditorAssetManager()->GetAssetRegistry();
			for (auto&& [handle, metadata] : assetRegistry)
			{
				if (filePath == std::filesystem::absolute(metadata.FilePath))
					return true;
			}

			return false;
		}

	}

	ContentBrowserPanel::ContentBrowserPanel()
		: m_CurrentDirectory(std::filesystem::current_path())
	{
		m_DirectoryIcon = TextureImporter::LoadTexture2D("../../EditorLayer/Resources/Icons/DirectoryIcon.png");
		m_FileIcon = TextureImporter::LoadTexture2D("../../EditorLayer/Resources/Icons/FileIcon.png");
		m_RefreshIcon = TextureImporter::LoadTexture2D("../../EditorLayer/Resources/Icons/RefreshIcon.png");

		m_DirectoryIconID = ImGuiLayer::AddTexture(*std::dynamic_pointer_cast<VulkanTexture>(m_DirectoryIcon));
		m_FileIconID = ImGuiLayer::AddTexture(*std::dynamic_pointer_cast<VulkanTexture>(m_FileIcon));
		m_RefreshIconID = ImGuiLayer::AddTexture(*std::dynamic_pointer_cast<VulkanTexture>(m_RefreshIcon));

		std::unique_ptr<Timer> timer = std::make_unique<Timer>("Asset Tree Creation");

		m_RootNode = new TreeNode;
		m_RootNode->FilePath = std::filesystem::current_path();
		UpdateAssetTree(m_RootNode);
	}

	ContentBrowserPanel::~ContentBrowserPanel()
	{
		delete m_RootNode;
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser");

		static TreeNode* tempNode = m_RootNode;
		if (m_AssetMode)
		{
			if (tempNode != m_RootNode)
			{
				if (ImGui::Button("Back"))
					tempNode = tempNode->ParentNode;

				ImGui::SameLine();
			}
		}
		else
		{
			if (m_CurrentDirectory != std::filesystem::current_path())
			{
				if (ImGui::Button("Back"))
					m_CurrentDirectory = m_CurrentDirectory.parent_path();

				ImGui::SameLine();
			}
		}

		if (ImGui::ImageButton("##RefreshButton", m_RefreshIconID, { 18.5f, 18.5f }, { 0, 0 }, { 1, 1 }))
		{
			auto oldNode = m_RootNode;
			m_RootNode = new TreeNode;
			m_RootNode->FilePath = std::filesystem::current_path();
			UpdateAssetTree(m_RootNode);

			tempNode = m_RootNode;
			delete oldNode;
		}

		ImGui::SameLine();

		ImVec4 buttonColor = m_AssetMode ? ImVec4{ 0.1f, 0.1f, 0.1f, 1.0f } : ImVec4{ 0.2f, 0.3f, 0.8f, 1.0f };
		ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, buttonColor);
		if (ImGui::Button("Show All Files"))
			m_AssetMode = !m_AssetMode;
		ImGui::PopStyleColor(2);

		float cellSize = m_ThumbnailSize + m_Padding;

		float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		ImGui::Columns(columnCount, nullptr, false);

		if (m_AssetMode)
		{
			for (auto& directoryNode : tempNode->ChildNodes)
			{
				const auto& path = directoryNode.FilePath;
				std::string filenameString = path.filename().string();

				ImGui::PushID(filenameString.c_str());
				ImTextureID icon = std::filesystem::is_directory(path) ? m_DirectoryIconID : m_FileIconID;

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
				ImGui::ImageButton(filenameString.c_str(), icon, { m_ThumbnailSize, m_ThumbnailSize }, { 0, 0 }, { 1, 1 });

				if (ImGui::BeginDragDropSource())
				{
					auto relativePath = std::filesystem::relative(path);
					const wchar_t* itemPath = relativePath.c_str();
					ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));
					ImGui::EndDragDropSource();
				}

				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					if (std::filesystem::is_directory(path))
						tempNode = &directoryNode;

					AssetType assetType = Utils::s_AssetExtensionMap[path.extension()];
					if (assetType == AssetType::Material)
					{
						auto relativePath = std::filesystem::relative(path);
						std::string pathStr = relativePath.generic_string();

						std::shared_ptr<MaterialAsset> materialAsset = AssetManager::GetAsset<MaterialAsset>(pathStr);
						m_MaterialEditor = std::make_shared<MaterialEditor>(materialAsset);
					}
				}

				ImGui::TextWrapped(filenameString.c_str());
				ImGui::NextColumn();

				ImGui::PopID();
			}
		}
		else
		{
			for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
			{
				const auto& path = directoryEntry.path();
				std::string filenameString = path.filename().string();

				ImGui::PushID(filenameString.c_str());
				ImTextureID icon = directoryEntry.is_directory() ? m_DirectoryIconID : m_FileIconID;

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
				ImGui::ImageButton(filenameString.c_str(), icon, { m_ThumbnailSize, m_ThumbnailSize }, { 0, 0 }, { 1, 1 });

				if (ImGui::BeginDragDropSource())
				{
					auto relativePath = std::filesystem::relative(path);
					const wchar_t* itemPath = relativePath.c_str();
					ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));
					ImGui::EndDragDropSource();
				}

				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					if (directoryEntry.is_directory())
						m_CurrentDirectory /= path.filename();

					AssetType assetType = Utils::s_AssetExtensionMap[path.extension()];
					if (assetType == AssetType::Material)
					{
						auto relativePath = std::filesystem::relative(path);
						std::string pathStr = relativePath.generic_string();

						std::shared_ptr<MaterialAsset> materialAsset = AssetManager::GetAsset<MaterialAsset>(pathStr);
						m_MaterialEditor = std::make_shared<MaterialEditor>(materialAsset);
					}
				}

				if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
					ImGui::OpenPopup("AssetImport");

				bool openMeshImportDialog = false, openRemoveAssetDialog = false;
				if (ImGui::BeginPopup("AssetImport"))
				{
					if (ImGui::MenuItem("Import"))
					{
						AssetType assetType = Utils::s_AssetExtensionMap[path.extension()];

						auto relativePath = std::filesystem::relative(path);
						std::string filepath = relativePath.generic_string();

						if (assetType == AssetType::Texture2D)
							AssetManager::ImportNewAsset<Texture2D>(filepath);
						if (assetType == AssetType::MeshAsset)
							openMeshImportDialog = true;
						if (assetType == AssetType::Mesh)
							VK_ERROR("AssetType Mesh is already in Registry!");
						if (assetType == AssetType::None)
							VK_ERROR("Extension type currently is not defined!");
					}

					if (ImGui::MenuItem("Remove"))
						openRemoveAssetDialog = true;

					ImGui::EndPopup();
				}

				RemoveAssetDialog(openRemoveAssetDialog, path);
				MeshImportDialog(openMeshImportDialog, path);

				ImGui::TextWrapped(filenameString.c_str());
				ImGui::NextColumn();

				ImGui::PopID();
			}
		}

		ImGui::Columns(1);

		if (m_MaterialEditor)
			m_MaterialEditor->OnImGuiRender();

		CreateMaterialDialog();

		// TODO: status bar
		ImGui::End();
	}

	void ContentBrowserPanel::MeshImportDialog(bool open, const std::filesystem::path& path)
	{
		if (open)
			ImGui::OpenPopup("Mesh Import Dialog");

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal("Mesh Import Dialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			std::shared_ptr<MeshSource> meshSource = nullptr;

			ImGui::Text("Importing a mesh is going to create a additional file which will store main asset handle");
			ImGui::TextColored({ 1.0f, 1.0f, 0.0f, 1.0f }, "NOTE: A material has to be created separately and assigned to mesh!");

			static char buffer[512];
			ImGui::Text("assets/meshes/");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetFontSize() * 25.0f);
			ImGui::InputText("##InputPath", buffer, sizeof(buffer));
			ImGui::SameLine();
			ImGui::Text(".vkmesh");

			ImGui::Separator();
			if (ImGui::Button("OK"))
			{
				std::string pathStr = { buffer };
				std::replace(pathStr.begin(), pathStr.end(), '/', '\\');
				std::filesystem::path filepath = pathStr;
				filepath.replace_extension(".vkmesh");

				filepath = "meshes" / filepath;

				meshSource = AssetManager::ImportNewAsset<MeshSource>(path.generic_string());
				AssetManager::CreateNewAsset<Mesh>(filepath.generic_string(), meshSource);
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}

	void ContentBrowserPanel::RemoveAssetDialog(bool open, const std::filesystem::path& path)
	{
		if (open)
			ImGui::OpenPopup("Remove Asset");

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal("Remove Asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			AssetType assetType = Utils::s_AssetExtensionMap[path.extension()];
			std::string filepath = path.generic_string();

			ImGui::Text("Are you sure, you want to delete %s?", filepath.data());

			ImGui::Separator();
			if (ImGui::Button("OK"))
			{
				bool removed = false;

				// TODO: Implement for other asset types
				if (assetType == AssetType::Texture2D)
					removed = AssetManager::RemoveAsset<Texture2D>(filepath);
				if (assetType == AssetType::Mesh)
					removed = AssetManager::RemoveAsset<Mesh>(filepath);
				if (assetType == AssetType::Material)
					removed = AssetManager::RemoveAsset<MaterialAsset>(filepath);

				// TODO: Here we should prompt another popup stating whether file has been deleted or not
				if (removed)
					VK_WARN("File {} is removed from ContentBrowser!", filepath);
				else
					VK_WARN("Unable to remove file {}! It could be referencing in memory!", filepath);

				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}

	void ContentBrowserPanel::CreateMaterialDialog()
	{
		bool openMaterialCreateDialog = false;
		if (ImGui::BeginPopupContextWindow("##CreateAsset", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverExistingPopup))
		{
			if (ImGui::BeginMenu("Create"))
			{
				if (ImGui::MenuItem("Material"))
					openMaterialCreateDialog = true;

				ImGui::EndMenu();
			}

			ImGui::EndPopup();
		}

		if (openMaterialCreateDialog)
			ImGui::OpenPopup("Create Material Dialog");

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal("Create Material Dialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			std::shared_ptr<MaterialAsset> materialAsset = nullptr;

			ImGui::Text("Creating a Material Asset will write to Asset Registry");

			static char buffer[512];
			ImGui::Text("assets/materials/");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetFontSize() * 25.0f);
			ImGui::InputText("##InputPath", buffer, sizeof(buffer));
			ImGui::SameLine();
			ImGui::Text(".vkmat");

			ImGui::Separator();
			if (ImGui::Button("OK"))
			{
				std::string pathStr = { buffer };
				std::replace(pathStr.begin(), pathStr.end(), '/', '\\');
				std::filesystem::path filepath = pathStr;
				filepath.replace_extension(".vkmat");

				filepath = "materials" / filepath;

				std::shared_ptr<Material> material = Material::Create(filepath.stem().string());
				materialAsset = AssetManager::CreateNewAsset<MaterialAsset>(filepath.generic_string(), material);
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}

	void ContentBrowserPanel::UpdateAssetTree(TreeNode* treeNode)
	{
		auto directoryIterator = std::filesystem::directory_iterator(treeNode->FilePath);

		for (const auto& directoryEntry : directoryIterator)
		{
			auto fileStatus = directoryEntry.status();
			switch (fileStatus.type())
			{
			case std::filesystem::file_type::directory:
			{
				auto& childNode = treeNode->ChildNodes.emplace_back();
				childNode.FilePath = directoryEntry.path();
				childNode.ParentNode = treeNode;
				UpdateAssetTree(&childNode);

				break;
			}
			default:
			{
				std::string pathString = directoryEntry.path().generic_string();
				if (Utils::IsAssetValid(pathString))
				{
					auto& childNode = treeNode->ChildNodes.emplace_back();
					childNode.FilePath = directoryEntry.path();
					childNode.ParentNode = treeNode;
				}

				break;
			}
			}
		}

		treeNode->ChildNodes.remove_if([](TreeNode& node)
		{
			return node.ChildNodes.empty() && std::filesystem::is_directory(node.FilePath);
		});
	}

}
