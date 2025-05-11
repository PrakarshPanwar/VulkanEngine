#pragma once

namespace VulkanCore {

	template<typename T, typename... Args>
	void HashCombine(size_t& seed, const T& v, const Args&... args)
	{
		seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		(HashCombine(seed, args), ...);
	}

}
