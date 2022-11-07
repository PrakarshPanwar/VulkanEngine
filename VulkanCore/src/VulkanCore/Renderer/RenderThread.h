#pragma once

namespace VulkanCore {

	class RenderThread
	{
	public:
		static void Init();

		template<typename FuncT>
		static void SubmitToThread(FuncT&& func)
		{
			std::unique_lock<std::mutex> submitLock(m_RTMutex);

			m_RTQueue.emplace_back(std::move(func));
			m_RTCondVar.notify_one();
		}

		static void WaitandDestroy();
	private:
		static void ThreadEntryPoint();
	private:
		static std::mutex m_RTMutex;
		static std::condition_variable m_RTCondVar;
		static std::vector<std::function<void()>> m_RTQueue;
		static std::jthread m_RenderThread;
		static bool m_RTShutDown;
	};

}