#pragma once

#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

namespace VulkanCore {

	class Log
	{
	public:
		static void Init();

		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};

}

// Core log macros
#define VK_CORE_TRACE(...)    ::VulkanCore::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define VK_CORE_INFO(...)     ::VulkanCore::Log::GetCoreLogger()->info(__VA_ARGS__)
#define VK_CORE_WARN(...)     ::VulkanCore::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define VK_CORE_ERROR(...)    ::VulkanCore::Log::GetCoreLogger()->error(__VA_ARGS__)
#define VK_CORE_CRITICAL(...) ::VulkanCore::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define VK_TRACE(...)         ::VulkanCore::Log::GetClientLogger()->trace(__VA_ARGS__)
#define VK_INFO(...)          ::VulkanCore::Log::GetClientLogger()->info(__VA_ARGS__)
#define VK_WARN(...)          ::VulkanCore::Log::GetClientLogger()->warn(__VA_ARGS__)
#define VK_ERROR(...)         ::VulkanCore::Log::GetClientLogger()->error(__VA_ARGS__)
#define VK_CRITICAL(...)      ::VulkanCore::Log::GetClientLogger()->critical(__VA_ARGS__)
