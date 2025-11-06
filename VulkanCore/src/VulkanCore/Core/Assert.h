#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define VK_PLATFORM_WINDOWS
#elif defined(__linux__)
#define VK_PLATFORM_LINUX
#else
#error "Unsupported Platform!"
#endif

#if defined(_MSC_VER)
#define DEBUG_BREAK __debugbreak()
#elif defined(__GNUC__)
#define DEBUG_BREAK __builtin_trap()
#endif

#define VK_CORE_ASSERT(check, ...) if (!(check)) { VK_CORE_ERROR(__VA_ARGS__); DEBUG_BREAK; }
#define VK_SLANG_ASSERT(result, check, ...) if (check) { VK_CORE_ERROR(__VA_ARGS__); if (result != 0) { DEBUG_BREAK; } }
#define VK_CHECK_RESULT(check, ...) if (check != VK_SUCCESS) { VK_CORE_ERROR(__VA_ARGS__); DEBUG_BREAK; }
#define VK_CHECK_WARN(check, ...) if (check != VK_SUCCESS) { VK_CORE_WARN(__VA_ARGS__); }
