#pragma once

namespace VulkanCore {

	class RenderThread
	{
	public:
		static void Init(uint16_t threadCount);
		static void ThreadEntryPoint(int i);

		template<typename FuncT>
		static void SubmitToThread(FuncT&& func)
		{
			std::unique_lock<std::mutex> mutexLk(m_Mutex);

			m_JobQueue.emplace_back(std::move(func));
			m_CondVar.notify_one();
		}

		static void WaitandDestroy();
	private:
		static std::mutex m_Mutex;
		static std::condition_variable m_CondVar;
		static std::vector<std::function<void()>> m_JobQueue;
		static std::vector<std::jthread> m_Threads;
		static bool m_ShutDown;

		friend class EditorLayer;
	};

}