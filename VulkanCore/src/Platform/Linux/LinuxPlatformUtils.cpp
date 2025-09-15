#include "vulkanpch.h"
#include "VulkanCore/Utils/PlatformUtils.h"
#include "VulkanCore/Core/Application.h"

#if defined(__linux__)
namespace VulkanCore {

	std::string FileDialogs::OpenFile(const char* filter)
	{
		std::string command = std::format("zenity --file-selection --title='Open File' --file-filter='{}'", filter);

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
		std::string command = std::format("zenity --file-selection --save --title='Save File' --file-filter='{}' --confirm-overwrite", filter);

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

	float PlatformTime::GetTime()
	{
		return static_cast<float>(glfwGetTime());
	}

}
#endif
