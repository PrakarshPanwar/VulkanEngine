#pragma once

namespace VulkanCore {

	class RenderThread
	{
	public:
		static void Init();

		template<typename FuncT>
		static void SubmitToThread(FuncT&& func)
		{
			std::unique_lock<std::mutex> submitLock(m_ThreadMutex);
			m_RenderCommandQueue.emplace_back(std::move(func));
		}

		static void NextFrame();
		static void Wait();
		static void WaitAndSet();
		static void WaitandDestroy();
		static void NotifyThread();

		static int GetThreadFrameIndex() { return m_ThreadFrameIndex; }
		bool CommandQueueEmpty() { return m_RenderCommandQueue.empty(); }
	private:
		static void ThreadEntryPoint();
	private:
		static std::mutex m_ThreadMutex;
		static std::condition_variable m_ConditionVariable;
		static std::vector<std::function<void()>> m_RenderCommandQueue;
		static std::jthread m_RenderThread;
		static int m_ThreadFrameIndex;
		static bool m_Running;
	};

}