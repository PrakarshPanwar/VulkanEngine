#pragma once

#define VK_CORE_ASSERT(check, ...) if (!(check)) { VK_CORE_ERROR(__VA_ARGS__); __debugbreak(); }
#define VK_CHECK_RESULT(check, ...) if (check != VK_SUCCESS) { VK_CORE_ERROR(__VA_ARGS__); __debugbreak(); }