#include "vulkanpch.h"
#include "RenderThread.h"
#include "../Core/Log.h"

#include <Windows.h>

namespace VulkanCore {

	std::mutex RenderThread::m_RTMutex;
	std::condition_variable RenderThread::m_RTCondVar;
	std::vector<std::function<void()>> RenderThread::m_RTQueue;
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
			{
				std::unique_lock<std::mutex> entryLock(m_RTMutex);

				while (!m_RTShutDown && m_RTQueue.empty())
					m_RTCondVar.wait(entryLock);

				if (m_RTQueue.empty())
				{
					VK_CORE_WARN("Render Thread(ID: {}) Terminates", m_RenderThread.get_id());
					return;
				}

				VK_CORE_WARN("Render Thread(ID: {}) performs Task", m_RenderThread.get_id());

				JobWork = std::move(m_RTQueue.front());
				m_RTQueue.erase(m_RTQueue.begin());
			}

			// Do the job without holding any locks
			JobWork();
		}
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

}