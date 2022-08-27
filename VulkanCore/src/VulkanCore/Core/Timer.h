#pragma once
#include <chrono>
#include "Log.h"

namespace VulkanCore {

	class Timer
	{
	public:
		Timer()
		{
			m_StartTimePoint = std::chrono::high_resolution_clock::now();
		}

		Timer(const std::string& FunctionName)
			: m_FunctionName(FunctionName)
		{
			m_StartTimePoint = std::chrono::high_resolution_clock::now();
		}

		~Timer()
		{
			Stop();
		}

		void Stop()
		{
			auto EndTimePoint = std::chrono::high_resolution_clock::now();
			auto Duration = std::chrono::duration_cast<std::chrono::nanoseconds>(EndTimePoint - m_StartTimePoint);

			if (m_FunctionName.empty())
				VK_CORE_TRACE("{0} ms({1})", Duration.count() * 0.000001, Duration);

			else
				VK_CORE_TRACE("{0} took {1} ms({2})", m_FunctionName, Duration.count() * 0.000001, Duration);
		}
	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimePoint;
		std::string m_FunctionName;
	};

}