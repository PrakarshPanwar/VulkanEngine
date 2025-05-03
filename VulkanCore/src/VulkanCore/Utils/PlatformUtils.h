#pragma once
#include <string>

namespace VulkanCore {

	class FileDialogs
	{
	public:
		static std::string OpenFile(const char* filter);
		static std::string SaveFile(const char* filter);
	};

	class WindowsTime
	{
	public:
		static float GetTime();
	};

}
