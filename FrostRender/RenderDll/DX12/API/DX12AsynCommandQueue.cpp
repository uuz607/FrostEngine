#include"pch.h"
#include"DX12CommandList.h"
#include"DX12AsynCommandQueue.h"

namespace FrostDX12
{
	void CAsyncCommandQueue::ExecuteCommandlist::Process(const TASK_ARGS& args)
	{
		args.pCommandListPool->GetD3D12CommandQueue()->ExecuteCommandLists(1, &pCommandList);
	}

	void CAsyncCommandQueue::ResetCommandlist::Process(const TASK_ARGS& args)
	{
		pCommandList->Reset();
	}

	void CAsyncCommandQueue::SignalFence::Process(const TASK_ARGS& args)
	{
		args.pCommandListPool->GetD3D12CommandQueue()->Signal(pFence, FenceValue);
	}

	void CAsyncCommandQueue::WaitForFence::Process(const TASK_ARGS& args)
	{
		args.pCommandListPool->GetD3D12CommandQueue()->Wait(pFence, FenceValue);
	}

	void CAsyncCommandQueue::WaitForFences::Process(const TASK_ARGS& args)
	{
		if (FenceValues[CMDQUEUE_GRAPHICS]) args.pCommandListPool->GetD3D12CommandQueue()->Wait(pFences[CMDQUEUE_GRAPHICS], FenceValues[CMDQUEUE_GRAPHICS]);
		if (FenceValues[CMDQUEUE_COMPUTE])	args.pCommandListPool->GetD3D12CommandQueue()->Wait(pFences[CMDQUEUE_COMPUTE],  FenceValues[CMDQUEUE_COMPUTE]);
		if (FenceValues[CMDQUEUE_COPY])		args.pCommandListPool->GetD3D12CommandQueue()->Wait(pFences[CMDQUEUE_COPY],		FenceValues[CMDQUEUE_COPY]);
	}

	void CAsyncCommandQueue::PresentBackbuffer::Process(const TASK_ARGS& args)
	{
		DWORD result = S_OK;

	#ifdef __dxgi1_3_h__
		if (pDesc->Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
		{
			// Check if the swapchain is ready to accept another frame
			HANDLE frameLatencyWaitableObject = pSwapChain->GetFrameLatencyWaitableObject();
			result = WaitForSingleObjectEx(frameLatencyWaitableObject, 0, true);
		}

		if (pDesc->Stereo && (pDesc->Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING))
		{
			Flags |= DXGI_PRESENT_ALLOW_TEARING;
			SyncInterval = 0;
		}
	#endif

		if (result == S_OK)
		{
			*pPresentResult = pSwapChain->Present(SyncInterval, Flags);
		}

		FrostInterlockedDecrement(args.QueueFramesCounter);
	}


	CAsyncCommandQueue::CAsyncCommandQueue()
		: m_pCmdListPool(nullptr)
		, m_QueuedFramesCounter(0)
		, m_bStopRequested(false)
		, m_TaskEvent(INT_MAX, 0)
	{

	}

	CAsyncCommandQueue::~CAsyncCommandQueue()
	{
		Clear();
	}

	void CAsyncCommandQueue::Init(CCommandListPool* pCommandListPool)
	{
		m_pCmdListPool = pCommandListPool;
		m_QueuedFramesCounter = 0;
		m_bStopRequested = false;
		m_bSleeping = true;
		
		GetISystem()->GetIThreadManager()->SpawnThread(this, "DX12 AsyncCommandQueue");
	}

	void CAsyncCommandQueue::Clear()
	{
		SignalStop();
		m_TaskEvent.Release();

		GetISystem()->GetIThreadManager()->JoinThread(this, THREAD_JOIN);

		m_pCmdListPool = nullptr;
	}

	void CAsyncCommandQueue::ExecuteCommandLists(UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists)
	{
		for (UINT i = 0; i < NumCommandLists; ++i)
		{
			SubmissionTask task;
			::memset(&task, 0, sizeof(task));

			task.type = TASK_TYPE_EXECUTE_COMMANDLIST;
			task.Task.ExecuteCommandList.pCommandList = ppCommandLists[i];

			AddTask<ExecuteCommandlist>(task);
		}
	}

	void CAsyncCommandQueue::ResetCommandList(CCommandList* pCommandList)
	{
		SubmissionTask task;
		::memset(&task, 0, sizeof(task));

		task.type = TASK_TYPE_RESET_COMMANDLIST; 
		task.Task.ResetCommandList.pCommandList = pCommandList;

		AddTask<ResetCommandlist>(task);
	}


	void CAsyncCommandQueue::Signal(ID3D12Fence* pFence, const UINT64 FenceValue)
	{
		SubmissionTask task;
		::memset(&task, 0, sizeof(task));

		task.type = TASK_TYPE_SIGNAL_FENCE;
		task.Task.SignalFence.pFence = pFence;
		task.Task.SignalFence.FenceValue = FenceValue;

		AddTask<SignalFence>(task);
	}

	void CAsyncCommandQueue::Wait(ID3D12Fence* pFence, const UINT64 FenceValue)
	{
		SubmissionTask task;
		::memset(&task, 0, sizeof(task));

		task.type = TASK_TYPE_WAIT_FOR_FENCE;
		task.Task.WaitForFence.pFence = pFence;
		task.Task.WaitForFence.FenceValue = FenceValue;

		AddTask<WaitForFence>(task);
	}

	void CAsyncCommandQueue::Wait(ID3D12Fence** pFences, const UINT64(&FenceValues)[CMDQUEUE_NUM])
	{
		SubmissionTask task;
		::memset(&task, 0, sizeof(task));

		task.type = TASK_TYPE_WAIT_FOR_FENCES; 
		task.Task.WaitForFences.pFences = pFences;
		task.Task.WaitForFences.FenceValues[CMDQUEUE_GRAPHICS] = FenceValues[CMDQUEUE_GRAPHICS];
		task.Task.WaitForFences.FenceValues[CMDQUEUE_COMPUTE]  = FenceValues[CMDQUEUE_COMPUTE];
		task.Task.WaitForFences.FenceValues[CMDQUEUE_COPY]	   = FenceValues[CMDQUEUE_COPY];

		AddTask<WaitForFences>(task);
	}

	#ifdef DX12_LINKEDADAPTER
	void CAsyncCommandQueue::SyncAdapters(ID3D12Fence* pFence, const UINT64 FenceValue)
	{
		SubmissionTask task;
		::memset(&task, 0, sizeof(task));

		task.type = TASK_TYPE_SYNC_ADAPTERS; 
		task.Task.SyncAdapters.pFence = pFence;
		task.Task.SyncAdapters.FenceValue = FenceValue;

		AddTask<SyncAdapter>(task);
	}
	#endif

	void CAsyncCommandQueue::Present(IDXGISwapChain3* pSwapChain, HRESULT* pPresentResult, UINT SyncInterval, UINT Flags, const DXGI_SWAP_CHAIN_DESC1* pDesc, UINT bufferIndex)
	{
		FrostInterlockedIncrement(&m_QueuedFramesCounter);

		SubmissionTask task;
		::memset(&task, 0, sizeof(task));

		task.type = TASK_TYPE_PRESENT_BACKBUFFER; 
		task.Task.PresentBackbuffer.pSwapChain = pSwapChain;
		task.Task.PresentBackbuffer.pPresentResult = pPresentResult;
		task.Task.PresentBackbuffer.Flags = Flags;
		task.Task.PresentBackbuffer.pDesc = pDesc;
		task.Task.PresentBackbuffer.SyncInterval = SyncInterval;

		AddTask<PresentBackbuffer>(task);

		{
			while (m_QueuedFramesCounter > MAX_FRAMES_GPU_LAG)
			{
				FrostThreadWin32::FrostYieldThread();
			}
		}
	}

	void CAsyncCommandQueue::FlushNextPresent()
	{
		const int numQueuedFrames = m_QueuedFramesCounter;
		if (numQueuedFrames > 0)
		{
			while (numQueuedFrames == m_QueuedFramesCounter)
			{
				FrostThreadWin32::FrostYieldThread();
			}
		}
	}

	void CAsyncCommandQueue::ThreadEntry()
	{
		const TASK_ARGS taskArgs = { m_pCmdListPool, &m_QueuedFramesCounter };
		SubmissionTask  task;

		while (!m_bStopRequested)
		{
			m_TaskEvent.Acquire();

			if (m_TaskQueue.dequeue(task))
			{
				switch (task.type)
				{
				case TASK_TYPE_EXECUTE_COMMANDLIST: 
					 task.Process<ExecuteCommandlist>(taskArgs);
					 break;
				case TASK_TYPE_RESET_COMMANDLIST:
					 task.Process<ResetCommandlist>(taskArgs);
					 break;
				case TASK_TYPE_SIGNAL_FENCE:
					 task.Process<SignalFence>(taskArgs);
					 break;
				case TASK_TYPE_WAIT_FOR_FENCE:
					 task.Process<WaitForFence>(taskArgs);
					 break;
				case TASK_TYPE_WAIT_FOR_FENCES:
					 task.Process<WaitForFences>(taskArgs);
					 break;
				case TASK_TYPE_PRESENT_BACKBUFFER:
					 task.Process<PresentBackbuffer>(taskArgs);
					 break;
				}
			}
		}
	}
}