#include <vector>
#include <memory>
#include <filesystem>

#include "ContentBrowserPanel.h"
#include "VulkanCore/Asset/AssetManager.h"
#include "VulkanCore/Core/ImGuiLayer.h"

#include <imgui.h>

namespace VulkanCore {

	static const std::filesystem::path g_AssetPath = "assets";

	ContentBrowserPanel::ContentBrowserPanel()
		: m_CurrentDirectory(g_AssetPath)
	{
		m_DirectoryIcon = std::make_shared<VulkanTexture>("../EditorLayer/Resources/Icons/DirectoryIcon.png");
		m_FileIcon = std::make_shared<VulkanTexture>("../EditorLayer/Resources/Icons/FileIcon.png");

		m_DirectoryIconID = ImGuiLayer::AddTexture(*m_DirectoryIcon);
		m_FileIconID = ImGuiLayer::AddTexture(*m_FileIcon);
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser");

		if (m_CurrentDirectory != std::filesystem::path(g_AssetPath))
		{
			if (ImGui::Button("Back"))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
		}

		float cellSize = m_ThumbnailSize + m_Padding;

		float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		ImGui::Columns(columnCount, nullptr, false);

		for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
		{
			const auto& path = directoryEntry.path();
			std::string filenameString = path.filename().string();

			ImGui::PushID(filenameString.c_str());
			ImTextureID icon = directoryEntry.is_directory() ? m_DirectoryIconID : m_FileIconID;

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::ImageButton((ImTextureID)icon, { m_ThumbnailSize, m_ThumbnailSize }, { 0, 0 }, { 1, 1 });

			if (ImGui::BeginDragDropSource())
			{
				auto relativePath = std::filesystem::relative(path, g_AssetPath);
				const wchar_t* itemPath = relativePath.c_str();
				ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));
				ImGui::EndDragDropSource();
			}

			ImGui::PopStyleColor();
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if (directoryEntry.is_directory())
					m_CurrentDirectory /= path.filename();

			}

			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			{
				//AssetManager::CreateNewAsset(filenameString, )
			}

			ImGui::TextWrapped(filenameString.c_str());

			ImGui::NextColumn();

			ImGui::PopID();
		}

		ImGui::Columns(1);

		// TODO: status bar
		ImGui::End();
	}

}
