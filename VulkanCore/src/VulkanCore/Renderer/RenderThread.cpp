#include "vulkanpch.h"
#include "RenderThread.h"
#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Renderer/Renderer.h"

#include <Windows.h>

namespace VulkanCore {

	std::mutex RenderThread::m_ThreadMutex;
	std::atomic<bool> RenderThread::m_RenderThreadAtomic;
	std::vector<std::function<void()>> RenderThread::m_RenderCommandQueue;
	std::jthread RenderThread::m_RenderThread;
	int RenderThread::m_ThreadFrameIndex = 0;
	bool RenderThread::m_Running;

	void RenderThread::Init()
	{
		m_Running = true;
		m_RenderThreadAtomic.store(false);
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
				std::scoped_lock swapLock(m_ThreadMutex);
				executeQueue.swap(m_RenderCommandQueue);
			}

			// Execute Command Queue
			for (const auto& executeCommand : executeQueue)
				executeCommand();

			m_RenderThreadAtomic.notify_one();

			m_RenderThreadAtomic.wait(false);
			m_RenderThreadAtomic.store(false);
		}
	}

	void RenderThread::NextFrame()
	{
		auto nextFrame = []
		{
			m_ThreadFrameIndex = (m_ThreadFrameIndex + 1) % 3;
		};

		SubmitToThread(nextFrame);
		m_RenderThreadAtomic.store(true);
		m_RenderThreadAtomic.notify_one();
	}

	void RenderThread::Wait()
	{
		m_RenderThreadAtomic.wait(true);
	}

	void RenderThread::WaitAndSet()
	{
		m_RenderThreadAtomic.wait(true);
	}

	void RenderThread::WaitAndDestroy()
	{
		{
			m_RenderThreadAtomic.store(true);
			m_RenderThreadAtomic.notify_one();

			std::scoped_lock waitLock(m_ThreadMutex);
			m_Running = false;
		}

		if (m_RenderThread.joinable())
			m_RenderThread.join();
	}

	void RenderThread::NotifyThread()
	{
		m_RenderThreadAtomic.notify_one();
	}

}