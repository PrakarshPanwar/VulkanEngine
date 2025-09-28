#include "vulkanpch.h"
#include "VulkanCore/Core/Log.h"
#include "TextureImporter.h"
#include "AssetManager.h"

#include <windows.h>
#include <stb_image.h>
#include <algorithm>
#include <winioctl.h>

namespace VulkanCore {

	std::shared_ptr<Texture2D> TextureImporter::ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata)
	{
		return LoadTexture2D(metadata.FilePath.string());
	}

	// NOTE: This can only be utilized when we have our own cubemap format
	std::shared_ptr<TextureCube> TextureImporter::ImportTextureCube(AssetHandle handle, const AssetMetadata& metadata)
	{
		return nullptr;
	}

	void TextureImporter::SerializeTexture2D(const AssetMetadata& metadata, std::shared_ptr<Asset> asset)
	{
		std::shared_ptr<Texture2D> texture = std::dynamic_pointer_cast<Texture2D>(asset);
	}

	void TextureImporter::SerializeTextureCube(const AssetMetadata& metadata, std::shared_ptr<Asset> asset)
	{

	}

	// TODO: For channels we have to create runtime Texture(similar to Mesh)
	// that will store channel format and Texture Path or maybe we have to switch our swapchain to UNORM format
	std::shared_ptr<Texture2D> TextureImporter::LoadTexture2D(const std::string& path)
	{
		int width = 0, height = 0, channels = 0;
		void* data = nullptr;

		TextureSpecification spec{};
		if (stbi_is_hdr(path.c_str()))
		{
			spec.Format = ImageFormat::RGBA32F;
			data = (uint8_t*)stbi_loadf(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		}
		else if (!path.empty())
		{
			std::string pathStr{};
			pathStr.resize(path.size());

			std::transform(path.begin(), path.end(), pathStr.begin(), [](char c) { return std::tolower(c); });

			bool isNormal = pathStr.find("nor") != std::string::npos;
			bool isARM = pathStr.find("arm") != std::string::npos || path.find("metallicRoughness") != std::string::npos;

			if (isNormal || isARM)
				spec.Format = ImageFormat::RGBA8_UNORM;
			else if (pathStr.find("disp") != std::string::npos)
				spec.Format = ImageFormat::R8_UNORM;

#if 0
			{
				HANDLE fileHandle = CreateFileA(path.c_str(),
					GENERIC_READ,
					FILE_SHARE_READ,
					nullptr,
					OPEN_EXISTING,
					FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
					nullptr
				);

				if (fileHandle == INVALID_HANDLE_VALUE)
				{
					VK_CORE_ERROR("Failed to open file: {0}, {1}", path, GetLastError());
					__debugbreak();
				}

				STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR alignmentDescriptor = {};
				alignmentDescriptor.

				OVERLAPPED overlapped = {};
				overlapped.Offset = 0;
				overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

				const DWORD bufferSize = 64 * 1024; // 64KB buffer
				BYTE* buffer = (BYTE*)_aligned_malloc(bufferSize, 4096);

				bool readResult = ReadFile(fileHandle, buffer, bufferSize, nullptr, &overlapped);
				if (!readResult && GetLastError() != ERROR_IO_PENDING)
				{
					VK_CORE_ERROR("ReadFile failed: {}", GetLastError());
					__debugbreak();
				}

				DWORD bytesRead = 0;
				if (!GetOverlappedResult(fileHandle, &overlapped, &bytesRead, TRUE))
				{
					VK_CORE_ERROR("GetOverlappedResult failed: {}", GetLastError());
					__debugbreak();
				}

				CloseHandle(fileHandle);
				CloseHandle(overlapped.hEvent);
				_aligned_free(buffer);
			}
#endif

			data = stbi_load(path.c_str(), &width, &height, &channels, spec.Format == ImageFormat::R8_UNORM ? STBI_grey : STBI_rgb_alpha);
		}

		spec.Width = width;
		spec.Height = height;
		spec.GenerateMips = false;

		std::shared_ptr<Texture2D> texture = Texture2D::Create(data, spec);
		return texture;
	}

}
