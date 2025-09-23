#pragma once
#include <string>

namespace VulkanCore {

	class FileDialogs
	{
	public:
		static std::string OpenFile(const char* filter);
		static std::string SaveFile(const char* filter);
	};

	class PlatformTime
	{
	public:
		static float GetTime();
	};

}
