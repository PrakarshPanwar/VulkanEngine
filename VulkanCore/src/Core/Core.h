#pragma once

#define VK_CORE_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)