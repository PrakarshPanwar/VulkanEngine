#pragma once
#include <atomic>

namespace VulkanCore {

	template<typename FuncT>
	concept SubmitLambdaConcept = requires
	{
		{ std::invoke_result_t<FuncT>() } -> std::same_as<void>;
	} && std::regular_invocable<FuncT>;

	class RenderThread
	{
	public:
		static void Init();
		static void ExecuteDeletionQueue();

		template<SubmitLambdaConcept FuncT>
		static void SubmitToThread(FuncT&& func)
		{
			std::scoped_lock submitLock(m_ThreadMutex);
			m_RenderCommandQueue.emplace_back(std::move(func));
		}

		template<SubmitLambdaConcept FuncT>
		static void SubmitToDeletion(FuncT&& func)
		{
			std::scoped_lock deletionLock(m_DeletionMutex);
			m_DeletionCommandQueue.emplace_back(std::move(func));
		}

		static void NextFrame();
		static void Wait();
		static void WaitAndSet();
		static void WaitAndDestroy();
		static void NotifyThread();
		static void SetAtomicFlag(bool dispatchFlag) { m_RenderThreadAtomic.store(dispatchFlag); }

		static int GetThreadFrameIndex() { return m_ThreadFrameIndex; }
		static bool IsCommandQueueEmpty() { return m_RenderCommandQueue.empty(); }
		static bool IsDeletionQueueEmpty() { return m_DeletionCommandQueue.empty(); }
	private:
		static void ThreadEntryPoint();
		static void ExecuteCommandQueue();
	private:
		static std::mutex m_ThreadMutex;
		static std::mutex m_DeletionMutex;
		static std::atomic<bool> m_RenderThreadAtomic;
		static std::vector<std::function<void()>> m_RenderCommandQueue;
		static std::vector<std::function<void()>> m_DeletionCommandQueue;
		static std::jthread m_RenderThread;
		static int m_ThreadFrameIndex;
		static bool m_Running;
	};

}