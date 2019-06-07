#pragma once

#include"Common\Interface\IThread.h"
#include"Common\3rdParty\concqueue-mpsc.hpp"
#include"Common\Thread\FrostThread_win32.h"

namespace FrostDX12
{
	class CCommandListPool;
	class CCommandList;

	class CAsyncCommandQueue : public IThread 
	{
	public:
		static const int MAX_FRAMES_GPU_LAG = 1;

		CAsyncCommandQueue();
		~CAsyncCommandQueue();

		void Init(CCommandListPool* pCommandListPool);
		void Clear();
		void FlushNextPresent();
		void SignalStop() { m_bStopRequested = true; }
		
		// Equates to the number of pending Present() calls
		int  GetQueuedFramesCount() const { return m_QueuedFramesCounter; }

		void Present(IDXGISwapChain3* pSwapChain, HRESULT* pPresentResult, UINT SyncInterval, UINT Flags, const DXGI_SWAP_CHAIN_DESC1* pDesc, UINT bufferIndex);
		void ResetCommandList(CCommandList* pCommandList);
		void ExecuteCommandLists(UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);
		void Signal(ID3D12Fence* pFence, const UINT64 Value);
		void Wait(ID3D12Fence* pFence, const UINT64 Value);
		void Wait(ID3D12Fence** pFences, const UINT64(&Values)[CMDQUEUE_NUM]);

	#ifdef DX12_LINKEDADAPTER
		void SyncAdapters(ID3D12Fence* pFence, const UINT64 Value);
	#endif

	private:

		enum TASK_TYPE
		{
			TASK_TYPE_EXECUTE_COMMANDLIST,
			TASK_TYPE_RESET_COMMANDLIST,
			TASK_TYPE_PRESENT_BACKBUFFER,
			TASK_TYPE_SIGNAL_FENCE,
			TASK_TYPE_WAIT_FOR_FENCE,
			TASK_TYPE_WAIT_FOR_FENCES,
			TASK_TYPE_SYNC_ADAPTERS
		};

		struct TASK_ARGS
		{
			CCommandListPool*   pCommandListPool;

			volatile int*       QueueFramesCounter;
		};

		struct ExecuteCommandlist
		{
			ID3D12CommandList* pCommandList;

			void               Process(const TASK_ARGS& args);
		};

		struct ResetCommandlist
		{
			CCommandList* pCommandList;

			void          Process(const TASK_ARGS& args);
		};

		struct SignalFence
		{
			ID3D12Fence* pFence;
			UINT64       FenceValue;

			void         Process(const TASK_ARGS& args);
		};

		struct WaitForFence
		{
			ID3D12Fence* pFence;
			UINT64       FenceValue;

			void         Process(const TASK_ARGS& args);
		};

		struct WaitForFences
		{
			ID3D12Fence** pFences;
			UINT64        FenceValues[CMDQUEUE_NUM];

			void          Process(const TASK_ARGS& args);
		};

		#ifdef DX12_LINKEDADAPTER
		struct SyncAdapter
		{
			ID3D12Fence* pFence;
			UINT64       FenceValue;

			void         Process(const TASK_ARGS& args);
		};
		#endif

		struct PresentBackbuffer
		{
			IDXGISwapChain3*			 pSwapChain;
			HRESULT*                     pPresentResult;
			UINT                         SyncInterval;
			UINT                         Flags;
			const DXGI_SWAP_CHAIN_DESC1* pDesc;

			void                         Process(const TASK_ARGS& args);
		};

		struct SubmissionTask
		{
			TASK_TYPE type;

			union 
			{
				ExecuteCommandlist ExecuteCommandList;
				ResetCommandlist   ResetCommandList;
				SignalFence        SignalFence;
				WaitForFence       WaitForFence;
				WaitForFences      WaitForFences;

			#ifdef DX12_LINKEDADAPTER
				SyncAdapter       SyncAdapters;
			#endif

				PresentBackbuffer  PresentBackbuffer;
			} Task;

			template<typename TaskType>
			void Process(const TASK_ARGS& args)
			{
				TaskType* pTask = reinterpret_cast<TaskType*>(&Task);
				pTask->Process(args);
			}
		};

		template<typename TaskType>
		void AddTask(SubmissionTask& task)
		{
			m_TaskQueue.enqueue(task);
			m_TaskEvent.Release();
		}

		void ThreadEntry() override;

		volatile int                             m_QueuedFramesCounter;
		volatile bool                            m_bStopRequested;
		volatile bool                            m_bSleeping;

		CCommandListPool*						 m_pCmdListPool;
		Concqueue::mpsc_queue_t<SubmissionTask>  m_TaskQueue;
		FrostFastSemaphore						 m_TaskEvent;
	};
}