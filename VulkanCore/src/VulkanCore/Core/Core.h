#pragma once
#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

#include "optick.h"
#include "tracy/Tracy.hpp"

#define VK_CORE_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)
#define VK_SET_TRACY_PROFILER 1

#if VK_SET_TRACY_PROFILER
enum class TracyZoneLabelColor
{
	Red		 = 0xFF0000,
	Green	 = 0x00FF00,
	Blue	 = 0x0000FF,
	NavyBlue = 0x000080,
	Gold	 = 0xFFD700,
	Purple	 = 0xBD63C5,
	Cyan	 = 0x00FFFF,
	White	 = 0xFFFFFF
};

#define VK_CORE_PROFILE_FN(fn, cl) ZoneScopedNC(fn, (uint32_t)cl)
#define VK_CORE_PROFILE() ZoneScopedC((uint32_t)TracyZoneLabelColor::NavyBlue)
#define VK_CORE_PROFILE_THREAD(td) tracy::SetThreadName(td)
#define VK_CORE_START_FRAME(fm) FrameMarkStart(fm)
#define VK_CORE_END_FRAME(fm) FrameMarkEnd(fm)
#else
// Optick
#define VK_CORE_PROFILE_FN(fn) OPTICK_EVENT(fn)
#define VK_CORE_PROFILE() OPTICK_EVENT(__FUNCTION__)
#define VK_CORE_PROFILE_THREAD(td) OPTICK_THREAD(td)
#define VK_CORE_BEGIN_FRAME(fm) OPTICK_FRAME(fm)
#define VK_CORE_START_FRAME(fm) ;
#define VK_CORE_END_FRAME(fm) ;
#endif
