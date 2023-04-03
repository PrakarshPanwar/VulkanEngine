#pragma once
#include <atomic>

namespace VulkanCore {

	class RenderThread
	{
	public:
		static void Init();

		template<typename FuncT>
		static void SubmitToThread(FuncT&& func)
		{
			std::scoped_lock submitLock(m_ThreadMutex);
			m_RenderCommandQueue.emplace_back(std::move(func));
		}

		static void NextFrame();
		static void Wait();
		static void WaitAndSet();
		static void WaitAndDestroy();
		static void NotifyThread();
		static void SetDispatchFlag(bool dispatchFlag) { m_RenderThreadAtomic.store(dispatchFlag); }

		static int GetThreadFrameIndex() { return m_ThreadFrameIndex; }
		bool CommandQueueEmpty() { return m_RenderCommandQueue.empty(); }
	private:
		static void ThreadEntryPoint();
	private:
		static std::mutex m_ThreadMutex;
		static std::atomic<bool> m_RenderThreadAtomic;
		static std::vector<std::function<void()>> m_RenderCommandQueue;
		static std::jthread m_RenderThread;
		static int m_ThreadFrameIndex;
		static bool m_Running;
	};

}