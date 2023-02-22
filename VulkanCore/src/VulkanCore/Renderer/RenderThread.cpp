#include "vulkanpch.h"
#include "RenderThread.h"
#include "VulkanCore/Core/Core.h"

#include <Windows.h>

namespace VulkanCore {

	std::mutex RenderThread::m_RTMutex;
	std::condition_variable RenderThread::m_RTCondVar;
	std::vector<std::function<void()>> RenderThread::m_RenderCommandQueue;
	std::jthread RenderThread::m_RenderThread;
	bool RenderThread::m_RTShutDown;

	void RenderThread::Init()
	{
		m_RTShutDown = false;
		m_RenderThread = std::jthread(std::bind(&RenderThread::ThreadEntryPoint));

		auto threadHandle = m_RenderThread.native_handle();

		SetThreadDescription(threadHandle, L"Render Thread");
	}

	void RenderThread::ThreadEntryPoint()
	{
		std::function<void()> JobWork;

		while (true)
		{
			VK_CORE_PROFILE_THREAD("Render Thread");

			{
				std::unique_lock<std::mutex> entryLock(m_RTMutex);
				m_RTCondVar.wait(entryLock, [] { return m_RTShutDown || !m_RenderCommandQueue.empty(); });

				if (m_RenderCommandQueue.empty())
				{
					VK_CORE_WARN("Render Thread(ID: {}) Terminates", m_RenderThread.get_id());
					return;
				}

				JobWork = std::move(m_RenderCommandQueue.front());
				m_RenderCommandQueue.erase(m_RenderCommandQueue.begin());
			}

			// Do the job without holding any locks
			JobWork();

			if (m_RenderCommandQueue.empty()) { m_RTCondVar.notify_one(); }
		}
	}

	void RenderThread::Wait()
	{
		{
			std::unique_lock<std::mutex> waitLock(m_RTMutex);
			m_RTCondVar.wait(waitLock, [] { return m_RenderCommandQueue.empty(); });
		}
	}

	void RenderThread::WaitAndSet()
	{
		{
			std::unique_lock<std::mutex> waitAndSetLock(m_RTMutex);
			m_RTShutDown = true;
			m_RTCondVar.notify_one();
		}

		m_RTShutDown = false;
	}

	void RenderThread::WaitandDestroy()
	{
		{
			std::unique_lock<std::mutex> waitLock(m_RTMutex);

			m_RTShutDown = true;
			m_RTCondVar.notify_all();
		}

		if (m_RenderThread.joinable())
			m_RenderThread.join();
	}

	// This process should take place in Render Thread
	void RenderThread::NotifyMainThread()
	{
		{
			m_RTCondVar.notify_one();
		}
	}

}