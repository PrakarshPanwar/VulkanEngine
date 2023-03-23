#pragma once
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanTexture.h"

namespace VulkanCore {

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void SetThumbnailSize(float thumbnailSize) { m_ThumbnailSize = thumbnailSize; }
		void SetPadding(float padding) { m_Padding = padding; }

		void OnImGuiRender();
	private:
		std::filesystem::path m_CurrentDirectory;

		std::shared_ptr<VulkanTexture> m_DirectoryIcon, m_FileIcon;
		VkDescriptorSet m_DirectoryIconID, m_FileIconID;

		float m_ThumbnailSize = 128.0f, m_Padding = 16.0f;
	};

}
