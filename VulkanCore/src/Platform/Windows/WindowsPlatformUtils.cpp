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
	    std::string command = "zenity --file-selection --title=\"Open File\"";

	    // Add file filter
	    if (filter) {
	        command += " --file-filter=\"";
	        command += filter;
	        command += "\"";
	    }

	    // Execute command and capture output
	    FILE* pipe = popen(command.c_str(), "r");
	    if (!pipe) {
	        return {}; // Failed to open pipe
	    }

	    char buffer[1024];
	    std::string result;

	    if (fgets(buffer, sizeof(buffer), pipe)) {
	        result = buffer;
	        // Remove trailing newline
	        if (!result.empty() && result.back() == '\n') {
	            result.pop_back();
	        }
	    }

	    int exit_code = pclose(pipe);

	    // Check if user canceled (exit code 1) or error occurred
	    if (exit_code != 0) {
	        return {}; // User canceled or error
	    }

	    return result;
	}

    std::string FileDialogs::SaveFile(const char* filter)
	{
	    std::string command = "zenity --file-selection --save --title=\"Save File\"";

	    // Add file filter if provided
	    if (filter) {
	        command += " --file-filter=\"";
	        command += filter;
	        command += "\"";
	    }

	    // Add confirmation for overwrite (zenity handles this automatically for --save)
	    command += " --confirm-overwrite";

	    // Execute command and capture output
	    FILE* pipe = popen(command.c_str(), "r");
	    if (!pipe) {
	        return {}; // Failed to open pipe
	    }

	    char buffer[1024];
	    std::string result;

	    if (fgets(buffer, sizeof(buffer), pipe)) {
	        result = buffer;
	        // Remove trailing newline
	        if (!result.empty() && result.back() == '\n') {
	            result.pop_back();
	        }
	    }

	    int exit_code = pclose(pipe);

	    // Check if user canceled (exit code 1) or error occurred
	    if (exit_code != 0) {
	        return {}; // User canceled or error
	    }

	    return result;
	}
#endif

	float WindowsTime::GetTime()
	{
		return static_cast<float>(glfwGetTime());
	}

}
