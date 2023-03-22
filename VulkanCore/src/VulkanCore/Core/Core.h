#pragma once
#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

#include "optick.h"

#define VK_CORE_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

// Optick
#define VK_CORE_PROFILE_FN(fn) OPTICK_EVENT(fn)
#define VK_CORE_PROFILE() OPTICK_EVENT(__FUNCTION__)
#define VK_CORE_PROFILE_THREAD(td) OPTICK_THREAD(td)
#define VK_CORE_BEGIN_FRAME(nm) OPTICK_FRAME(nm)