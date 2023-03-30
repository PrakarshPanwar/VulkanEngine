#include "vulkanpch.h"
#include "RenderThread.h"
#include "VulkanCore/Core/Core.h"

#include <Windows.h>

namespace VulkanCore {

	std::mutex RenderThread::m_ThreadMutex;
	std::condition_variable RenderThread::m_ConditionVariable;
	std::vector<std::function<void()>> RenderThread::m_RenderCommandQueue;
	std::jthread RenderThread::m_RenderThread;
	int RenderThread::m_ThreadFrameIndex = 0;
	bool RenderThread::m_Running;

	void RenderThread::Init()
	{
		m_Running = true;
		m_RenderThread = std::jthread(std::bind(&RenderThread::ThreadEntryPoint));

		auto threadHandle = m_RenderThread.native_handle();

		SetThreadDescription(threadHandle, L"Render Thread");
	}

	void RenderThread::ThreadEntryPoint()
	{
		while (m_Running)
		{
			VK_CORE_PROFILE_THREAD("Render Thread");

			// Swap Queues
			std::vector<std::function<void()>> executeQueue;
			{
				std::unique_lock swapLock(m_ThreadMutex);
				executeQueue.swap(m_RenderCommandQueue);
			}

			// Execute Command Queue
			for (const auto& executeCommand : executeQueue)
				executeCommand();

			m_ConditionVariable.notify_one();

			std::unique_lock waitLock(m_ThreadMutex);
			m_ConditionVariable.wait(waitLock);
		}
	}

	void RenderThread::NextFrame()
	{
		auto nextFrame = []
		{
			VK_CORE_WARN("Proceeding to Next Frame");
			m_ThreadFrameIndex = (m_ThreadFrameIndex + 1) % 3;
		};

		SubmitToThread(nextFrame);
	}

	void RenderThread::Wait()
	{
		std::unique_lock<std::mutex> waitLock(m_ThreadMutex);
		m_ConditionVariable.wait(waitLock);
	}

	void RenderThread::WaitAndSet()
	{
		{
			std::unique_lock<std::mutex> waitAndSetLock(m_ThreadMutex);
			m_Running = true;
			m_ConditionVariable.notify_one();
		}

		m_Running = false;
	}

	void RenderThread::WaitandDestroy()
	{
		{
			std::unique_lock<std::mutex> waitLock(m_ThreadMutex);

			m_Running = false;
			m_ConditionVariable.notify_one();
		}

		if (m_RenderThread.joinable())
			m_RenderThread.join();
	}

	void RenderThread::NotifyThread()
	{
		m_ConditionVariable.notify_one();
	}

}