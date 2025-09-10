#include "vulkanpch.h"
#include "VulkanCore/Utils/PlatformUtils.h"
#include "VulkanCore/Core/Application.h"

#if defined(__WIN64__)
#include <Windows.h>
#include <commdlg.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

namespace VulkanCore {
#ifdef _WIN32
	std::string FileDialogs::OpenFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		CHAR currentDir[256] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		//ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get()->GetWindowsWindow()->GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		if (GetCurrentDirectoryA(256, currentDir))
			ofn.lpstrInitialDir = currentDir;
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn) == TRUE)
			return ofn.lpstrFile;

		return {};
	}

	std::string FileDialogs::SaveFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		CHAR currentDir[256] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		//ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get()->GetWindowsWindow()->GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		if (GetCurrentDirectoryA(256, currentDir))
			ofn.lpstrInitialDir = currentDir;
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

		// Sets the default extension by extracting it from the filter
		ofn.lpstrDefExt = strchr(filter, '\0') + 1;

		if (GetSaveFileNameA(&ofn) == TRUE)
			return ofn.lpstrFile;

		return {};
	}
#elif defined(__linux__)
    std::string FileDialogs::OpenFile(const char* filter)
    {
	    std::string command = std::format("zenity --file-selection --title=\"Open File\" --file-filter='{}'", filter);

	    // Execute command and capture output
	    FILE* pipe = popen(command.c_str(), "r");
	    if (!pipe)
	        return {}; // Failed to open pipe

	    std::string result;
	    char buffer[1024];
	    if (fgets(buffer, sizeof(buffer), pipe))
	    {
	        result = buffer;
	        if (!result.empty() && result.back() == '\n')
	            result.pop_back();
	    }

	    // Check if user cancelled (exit code 1) or error occurred
	    int exit_code = pclose(pipe);
	    if (exit_code != 0)
	        return {}; // User cancelled or error

	    return result;
	}

    std::string FileDialogs::SaveFile(const char* filter)
	{
	    std::string command = std::format("zenity --file-selection --save --title=\"Save File\" --file-filter='{}' --confirm-overwrite", filter);

	    // Execute command and capture output
	    FILE* pipe = popen(command.c_str(), "r");
	    if (!pipe)
	        return {}; // Failed to open pipe

	    std::string result;
	    char buffer[1024];
	    if (fgets(buffer, sizeof(buffer), pipe))
	    {
	        result = buffer;
	        if (!result.empty() && result.back() == '\n')
	            result.pop_back();
	    }

	    // Check if user cancelled (exit code 1) or error occurred
	    int exit_code = pclose(pipe);
	    if (exit_code != 0)
	        return {}; // User cancelled or error

	    return result;
	}
#endif

	float WindowsTime::GetTime()
	{
		return static_cast<float>(glfwGetTime());
	}

}
