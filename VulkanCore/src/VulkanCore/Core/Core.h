#pragma once
#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

#define VK_CORE_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)