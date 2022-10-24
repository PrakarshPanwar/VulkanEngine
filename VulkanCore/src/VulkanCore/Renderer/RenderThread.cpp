#include "vulkanpch.h"
#include "RenderThread.h"
#include "../Core/Log.h"

namespace VulkanCore {

	std::mutex RenderThread::m_Mutex;
	std::condition_variable RenderThread::m_CondVar;
	std::vector<std::function<void()>> RenderThread::m_JobQueue;
	std::vector<std::jthread> RenderThread::m_Threads;
	bool RenderThread::m_ShutDown;

	void RenderThread::Init(uint16_t threadCount)
	{
		m_ShutDown = false;
		m_Threads.reserve(threadCount);

		for (int i = 0; i < threadCount; i++)
			m_Threads.emplace_back(std::bind(&RenderThread::ThreadEntryPoint, i));
	}

	void RenderThread::ThreadEntryPoint(int i)
	{
		std::function<void()> JobWork;

		while (true)
		{
			{
				std::unique_lock<std::mutex> CondVarMutex(m_Mutex);

				while (!m_ShutDown && m_JobQueue.empty())
					m_CondVar.wait(CondVarMutex);

				if (m_JobQueue.empty())
				{
					VK_CORE_WARN("Thread {} Terminates", m_Threads.at(i).get_id());
					return;
				}

				VK_CORE_WARN("Thread {} does a Job", m_Threads.at(i).get_id());

				JobWork = std::move(m_JobQueue.front());
				m_JobQueue.erase(m_JobQueue.begin());
			}

			// Do the job without holding any locks
			JobWork();
		}
	}

	void RenderThread::WaitandDestroy()
	{
		{
			if (m_Threads.empty())
				return;

			std::unique_lock<std::mutex> threadLock(m_Mutex);

			m_ShutDown = true;
			m_CondVar.notify_all();
		}

		for (auto& qthread : m_Threads)
			qthread.join();

		m_Threads.clear();
	}

}