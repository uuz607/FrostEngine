#include"pch.h"
#include"DX12CommandList.h"
#include"DX12RootSignature.h"
#include"DX12Resource.h"

namespace FrostDX12
{
#pragma region CCommandList Implementation

#pragma region IFrostCommandList Interface
	void CCommandList::AddRef()
	{
		::_InterlockedIncrement(&m_RefCount);
	}

	void CCommandList::Release()
	{
		long refCount = ::_InterlockedDecrement(&m_RefCount);
		if (!refCount)
		{
			delete this;
		}
	}

	UINT64 CCommandList::SignalFenceOnGPU()
	{
		m_Pool.SignalFenceOnGPU(m_CurrentFenceValue);
		return m_CurrentFenceValue;
	}

	UINT64 CCommandList::SignalFenceOnCPU()
	{
		m_Pool.SignalFenceOnCPU(m_CurrentFenceValue);
		return m_CurrentFenceValue;
	}

	void CCommandList::WaitForFinishOnGPU(
		const UINT64 FenceValue,
		const int id) const
	{
		m_Pool.WaitForFenceOnGPU(FenceValue, id);
	}


	void CCommandList::WaitForFinishOnCPU() const
	{
		DX12_ASSERT(
			m_State == COMMAND_LIST_STATE_SUBMITTED, "GPU fence waits for itself to be complete: deadlock imminent!");

		m_Pool.WaitForFenceOnCPU(m_CurrentFenceValue);
	}

	void CCommandList::ResourceBarrier(
		IFrostResource* pResource,
		const D3D12_RESOURCE_STATES& DesiredState,
		UINT SubResource,
		const D3D12_RESOURCE_BARRIER_FLAGS& Flags)
	{
		if (pResource)
		{
			m_ResourceSet.insert(static_cast<CResource*>(pResource));

			static_cast<CResource*>(pResource)->TransitionBarrier(this, DesiredState, SubResource, Flags);

			m_nCommands += COMMAND_LIST_OPERATION_RESOURCE;
		}
	}

	void CCommandList::AliasBarrier(IFrostResource* pResourceBefore, IFrostResource* pResourceAfter)
	{
		ID3D12Resource* pResourceBefore_ =
			pResourceBefore != nullptr ? static_cast<CResource*>(pResourceBefore)->GetD3D12Resource() : nullptr;
		ID3D12Resource* pResourceAfter_ =
			pResourceAfter != nullptr ? static_cast<CResource*>(pResourceAfter)->GetD3D12Resource() : nullptr;

		m_ResourceBarrierHeap.emplace_back(CD3DX12_RESOURCE_BARRIER::Aliasing(pResourceBefore_, pResourceAfter_));

		m_nCommands += COMMAND_LIST_OPERATION_RESOURCE;
	}

	void CCommandList::SetVertexBuffers(
		UINT StartSlot,
		UINT NumViews,
		IFrostView** ppViews)
	{
		std::vector<D3D12_VERTEX_BUFFER_VIEW> vbviews;
		vbviews.reserve(NumViews);

		for (UINT i = 0; i < NumViews; ++i)
		{
			CView* pView = static_cast<CView*>(ppViews[i]);

			vbviews.emplace_back(pView->GetVBVDesc());
		}

		FlushResourceBarriers();

		m_pCmdList->IASetVertexBuffers(StartSlot, NumViews, vbviews.data());

		m_nCommands += COMMAND_LIST_OPERATION_RESOURCE;
	}

	void CCommandList::SetIndexBuffer(IFrostView* pView)
	{
		CView* pView_ = static_cast<CView*>(pView);

		FlushResourceBarriers();

		m_pCmdList->IASetIndexBuffer(&(pView_->GetIBVDesc()));

		m_nCommands += COMMAND_LIST_OPERATION_RESOURCE;
	}

	void CCommandList::SetPipelineState(IFrostPSO* pso)
	{
		m_pCmdList->SetPipelineState(
			static_cast<CPSO*>(pso)->GetD3D12PipelineState());

		m_nCommands += COMMAND_LIST_OPERATION_CONFIGURE;
	}

	void CCommandList::SetGraphicsRootSignature(IFrostRootSignature* pRootSignature)
	{
		m_pCmdList->SetGraphicsRootSignature(
			static_cast<CRootSignature*>(pRootSignature)->GetD3D12RootSignature());

		m_nCommands += COMMAND_LIST_OPERATION_CONFIGURE;
	}

	void CCommandList::SetComputeRootSignature(IFrostRootSignature* pRootSignature)
	{
		m_pCmdList->SetComputeRootSignature(
			static_cast<CRootSignature*>(pRootSignature)->GetD3D12RootSignature());

		m_nCommands += COMMAND_LIST_OPERATION_CONFIGURE;
	}

	void CCommandList::SetGraphicsDescriptorTable(UINT RootParameterIndex, IFrostView* pView)
	{
		CView* pView_ = static_cast<CView*>(pView);

		FlushResourceBarriers();

		m_pCmdList->SetGraphicsRootDescriptorTable(RootParameterIndex, pView_->GetGPUDescriptorHandle());

		m_nCommands += COMMAND_LIST_OPERATION_RESOURCE;
	}

	void CCommandList::SetGraphicsDescriptorTable(UINT RootParameterIndex, IFrostSamplerState* pSampler)
	{
		m_pCmdList->SetGraphicsRootDescriptorTable(
			RootParameterIndex, static_cast<CSamplerState*>(pSampler)->GetGPUDescriptorHandle());

		m_nCommands += COMMAND_LIST_OPERATION_CONFIGURE;
	}

	void CCommandList::SetRenderTargets(
		UINT NumRenderTargetDescriptors,
		IFrostView** pRenderTargets,
		BOOL RTsSingleHandleToDescriptorRange,
		IFrostView* pDepthStencil)
	{
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargetDescriptors;
		renderTargetDescriptors.reserve(NumRenderTargetDescriptors);

		for (UINT i = 0; i < NumRenderTargetDescriptors; ++i)
		{
			CView* pView = static_cast<CView*>(pRenderTargets[i]);

			renderTargetDescriptors.emplace_back(pView->GetCPUDescriptorHandle());
		}

		FlushResourceBarriers();

		m_pCmdList->OMSetRenderTargets(
			NumRenderTargetDescriptors,
			renderTargetDescriptors.data(),
			RTsSingleHandleToDescriptorRange,
			&(static_cast<CView*>(pDepthStencil)->GetCPUDescriptorHandle()));

		m_nCommands += COMMAND_LIST_OPERATION_RESOURCE;
	}

	void CCommandList::SetStencilRef(UINT StencilRef)
	{
		m_pCmdList->OMSetStencilRef(StencilRef);

		m_nCommands += COMMAND_LIST_OPERATION_CONFIGURE;
	}

	void CCommandList::SetViewports(UINT NumViewports, const D3D12_VIEWPORT* pViewports)
	{
		m_pCmdList->RSSetViewports(NumViewports, pViewports);

		m_nCommands += COMMAND_LIST_OPERATION_CONFIGURE;
	}

	void CCommandList::SetScissorRects(UINT NumRects, const D3D12_RECT* pRects)
	{
		m_pCmdList->RSSetScissorRects(NumRects, pRects);

		m_nCommands += COMMAND_LIST_OPERATION_CONFIGURE;
	}

	void CCommandList::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology)
	{
		m_pCmdList->IASetPrimitiveTopology(PrimitiveTopology);

		m_nCommands += COMMAND_LIST_OPERATION_CONFIGURE;
	}

	void CCommandList::SetDescriptorHeaps(IFrostDescriptorAllocator* heaps)
	{
		CDescriptorAllocator* pDescAlloca = static_cast<CDescriptorAllocator*>(heaps);

		ID3D12DescriptorHeap* pDescriptorHeaps[] =
		{
			pDescAlloca->GetCbvSrvUavHeap()->GetD3D12DescriptorHeap(),
			pDescAlloca->GetSamplerHeap()->GetD3D12DescriptorHeap()
		};

		m_pCmdList->SetDescriptorHeaps(_countof(pDescriptorHeaps), pDescriptorHeaps);

		m_nCommands += COMMAND_LIST_OPERATION_CONFIGURE;
	}

	void CCommandList::ClearDepthStencilView(
		IFrostView* pDepthStencilView,
		D3D12_CLEAR_FLAGS ClearFlags,
		float depthValue,
		UINT StencilValue,
		UINT NumRects,
		const D3D12_RECT* pRect)
	{
		CView* pView = static_cast<CView*>(pDepthStencilView);
		DX12_ASSERT(INVALID_CPU_DESCRIPTOR_HANDLE != pView->GetCPUDescriptorHandle(), "View has no descriptor handle, that is not allowed!");

		/*
		TODO:
		if we know early that the resource(s) will be PRESENT,
		we can begin the barrier early and end it here.
		*/

		FlushResourceBarriers();

		m_pCmdList->ClearDepthStencilView(
			pView->GetCPUDescriptorHandle(), ClearFlags,
			depthValue, StencilValue, NumRects, pRect);

		m_nCommands += COMMAND_LIST_OPERATION_RESOURCE;
	}

	void CCommandList::ClearRenderTargetView(
		IFrostView* pRenderTargetView,
		const FLOAT rgba[4],
		UINT NumRects,
		const D3D12_RECT* pRects)
	{
		CView* pView = static_cast<CView*>(pRenderTargetView);

		DX12_ASSERT(
			INVALID_CPU_DESCRIPTOR_HANDLE != pView->GetCPUDescriptorHandle(),
			"View has no descriptor handle, that is not allowed!");

		CResource* pRes = pView->GetDX12Resource();
		pRes->VerifyBackBuffer();

		FlushResourceBarriers();

		m_pCmdList->ClearRenderTargetView(
			pView->GetCPUDescriptorHandle(), rgba, NumRects, pRects);

		m_nCommands += COMMAND_LIST_OPERATION_RESOURCE;
	}

	void CCommandList::ClearUnorderedAccessView(
		IFrostView* pUnorderedAccessView,
		const UINT rgba[4],
		UINT NumRects,
		const D3D12_RECT* pRects)
	{
		CView* pView = static_cast<CView*>(pUnorderedAccessView);

		DX12_ASSERT(
			INVALID_CPU_DESCRIPTOR_HANDLE != pView->GetCPUDescriptorHandle(),
			"View has no descriptor handle, that is not allowed!");

		CResource* pRes = pView->GetDX12Resource();

		FlushResourceBarriers();

		m_pCmdList->ClearUnorderedAccessViewUint(
			pView->GetGPUDescriptorHandle(), pView->GetCPUDescriptorHandle(),
			pView->GetD3D12Resource(), rgba, NumRects, pRects);

		m_nCommands += COMMAND_LIST_OPERATION_RESOURCE;
	}

	void CCommandList::ClearUnorderedAccessView(
		IFrostView* pUnorderedAccessView,
		const FLOAT rgba[4],
		UINT NumRects,
		const D3D12_RECT* pRects)
	{
		CView* pView = static_cast<CView*>(pUnorderedAccessView);

		DX12_ASSERT(
			INVALID_CPU_DESCRIPTOR_HANDLE != pView->GetCPUDescriptorHandle(),
			"View has no descriptor handle, that is not allowed!");

		CResource* pRes = pView->GetDX12Resource();

		FlushResourceBarriers();

		m_pCmdList->ClearUnorderedAccessViewFloat(
			pView->GetGPUDescriptorHandle(), pView->GetCPUDescriptorHandle(),
			pView->GetD3D12Resource(), rgba, NumRects, pRects);

		m_nCommands += COMMAND_LIST_OPERATION_RESOURCE;
	}

	void CCommandList::DrawInstanced(
		UINT VertexCountPerInstance,
		UINT InstanceCount,
		UINT StartVertexLocation,
		UINT StartInstanceLocation)
	{
		FlushResourceBarriers();

		m_pCmdList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);

		m_nCommands += COMMAND_LIST_OPERATION_RESOURCE;
	}

	void CCommandList::DrawIndexedInstanced(
		UINT IndexCountPerInstance,
		UINT InstanceCount,
		UINT StartIndexLocation,
		UINT BaseVertexLocation,
		UINT StartInstanceLocation)
	{
		FlushResourceBarriers();

		m_pCmdList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

		m_nCommands += COMMAND_LIST_OPERATION_RESOURCE;
	}

	void CCommandList::UpdateSubresources(
		IFrostResource* pFrostDestResource,
		IFrostResource* pFrostIntermedia,
		UINT64 IntermediateOffset,
		UINT FirstSubresource,
		UINT NumSubResources,
		D3D12_SUBRESOURCE_DATA* pSrcData)
	{
		ID3D12Resource* pDestResource = static_cast<CResource*>(pFrostDestResource)->GetD3D12Resource();
		ID3D12Resource* pIntermedia = static_cast<CResource*>(pFrostIntermedia)->GetD3D12Resource();


		//Generally, update subresources in copy queue.So, set resource state to D3D12_RESOURCE_STATE_COMMON.
		if (D3D12_COMMAND_LIST_TYPE_COPY == m_ListType)
		{
			ResourceBarrier(pFrostDestResource, D3D12_RESOURCE_STATE_COMMON);
		}
		else
		{
			ResourceBarrier(pFrostDestResource, D3D12_RESOURCE_STATE_COPY_DEST);
		}

		FlushResourceBarriers();

		::UpdateSubresources(
			m_pCmdList, pDestResource, pIntermedia,
			IntermediateOffset, FirstSubresource, NumSubResources, pSrcData);

		m_nCommands += COMMAND_LIST_OPERATION_RESOURCE;
	}
#pragma endregion

#pragma region Get() & Set() & Is()

#pragma endregion

	CCommandList::CCommandList(CCommandListPool& pool) :
		m_Pool(pool),
		m_pDevice(pool.GetDevice()),
		m_pCmdQueue(nullptr),
		m_pCmdAllocator(nullptr),
		m_pCmdList(nullptr),
		m_ListType(pool.GetD3D12QueueType()),
		m_CurrentFenceValue(0),
		m_State(COMMAND_LIST_STATE_FREE),
		m_nodeMask(pool.GetNodeMask()),
		m_RefCount(0)
	{
		m_ResourceBarrierHeap.reserve(256);
	}

	CCommandList::~CCommandList()
	{

	}

	bool CCommandList::Init(UINT64 CurrentFenceValue)
	{
		m_pDevice = m_Pool.GetDevice();

		m_CurrentFenceValue = CurrentFenceValue;

		if (!m_pCmdQueue)
		{
			m_pCmdQueue = m_Pool.GetD3D12CommandQueue();
		}

		D3D12_COMMAND_LIST_TYPE cmdListType = m_pCmdQueue->GetDesc().Type;

		if (!m_pCmdAllocator)
		{
			if (FAILED(m_pDevice->CreateCommandAllocator(cmdListType, IID_PPV_ARGS(&m_pCmdAllocator))))
			{
				DX12_ERROR("Could not create command allocator!");
				return false;
			}
		}

		if (!m_pCmdList)
		{
			if (FAILED(m_pDevice->CreateCommandList(
				m_nodeMask, cmdListType, m_pCmdAllocator, nullptr, IID_PPV_ARGS(&m_pCmdList))))
			{
				DX12_ERROR("Could not create pre-command list!");
				return false;
			}
		}

		return true;
	}

	void CCommandList::Register()
	{
		UINT64 nextFenceValue = m_Pool.GetCurrentFenceValue() + 1;
		/*
		Increment fence on allocation, this has the effect that
		acquired CommandLists need to be submitted in-order to prevent
		dead-locking.
		*/
		m_Pool.SetCurrentFenceValue(nextFenceValue);

		Init(nextFenceValue);

		m_State = COMMAND_LIST_STATE_STARTED;
	}

	void CCommandList::Schedule()
	{
		if (m_State < COMMAND_LIST_STATE_SCHEDULED)
		{
			m_State = COMMAND_LIST_STATE_SCHEDULED;

			const bool bRecyclable = (m_Pool.GetCurrentFenceValue() == m_CurrentFenceValue);
			if (bRecyclable & !IsUtilized())
			{
				// Rewind fence value and don't declare submitted
				m_Pool.SetCurrentFenceValue(m_CurrentFenceValue - 1);

				// The command-list doesn't "exist" now (makes all IsFinished() checks return true)
				m_CurrentFenceValue = 0;
			}
		}
	}
	void CCommandList::End()
	{
		FlushResourceBarriers();

		/*
		Some resource states may decay back to D3D12_RESOURCE_STATE_COMMON.
		Decay does not occur between command lists executed together in the same ExecuteCommandLists call.
		*/
		for (auto res : m_ResourceSet)
		{
			res->DecayTransitionBarrier(this);
		}

		if (FAILED(m_pCmdList->Close()))
		{
			DX12_LOG(true, "Could not close command list!");
		}

		m_State = COMMAND_LIST_STATE_COMPLETED;
	}

	void CCommandList::Submit()
	{
		if (IsUtilized())
		{
			// Then inject the Execute() which is possibly blocked by the Wait()
			ID3D12CommandList* ppCommandLists[] = { m_pCmdList };
			// TODO: allow to submit multiple command-lists in one go
			m_Pool.GetAsyncCommandQueue()->ExecuteCommandLists(1, ppCommandLists);

			SignalFenceOnGPU();
		}
		
		m_State = COMMAND_LIST_STATE_SUBMITTED;
	}

	void CCommandList::Clear()
	{
		m_State = COMMAND_LIST_STATE_CLEARING;
		m_Pool.GetAsyncCommandQueue()->ResetCommandList(this);
	}

	bool CCommandList::Reset()
	{
		// reset the allocator before the list re-occupies it, otherwise the whole state of the allocator starts leaking
		if (m_pCmdAllocator)
		{
			m_pCmdAllocator->Reset();
		}

		// make the list re-occupy this allocator, reseting the list will _not_ reset the allocator
		if (m_pCmdList)
		{	
			m_pCmdList->Reset(m_pCmdAllocator, nullptr);
		}

		m_State = COMMAND_LIST_STATE_FREE;

		return true;
	}

	void CCommandList::ResourceBarrier(D3D12_RESOURCE_BARRIER& ResourceBarrier)
	{
		auto pred = [&](const D3D12_RESOURCE_BARRIER& Barrier)
		{
			return  Barrier.Type == ResourceBarrier.Type &&
				Barrier.Transition.pResource == ResourceBarrier.Transition.pResource &&
				Barrier.Transition.Subresource == ResourceBarrier.Transition.Subresource;
		};

		auto iter_barrier = std::find_if(
			m_ResourceBarrierHeap.begin(), m_ResourceBarrierHeap.end(), pred);

		if (iter_barrier != m_ResourceBarrierHeap.end())
		{
			if (D3D12_RESOURCE_BARRIER_TYPE_TRANSITION == ResourceBarrier.Type)
			{
				if (D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY == (*iter_barrier).Flags &&
					D3D12_RESOURCE_BARRIER_FLAG_END_ONLY == ResourceBarrier.Flags)
				{
					(*iter_barrier).Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

					return;
				}

				/*
				In this case:
				The transition barrier: A->B.
				The other one: ? ->B.

				If both of them are adjacent(A->B,B->B), such a transition is invalid.
				*/
				if (ResourceBarrier.Transition.StateAfter == (*iter_barrier).Transition.StateAfter)
				{
					return;
				}

				/*
				In this case:
				The transition barrier: A->B.
				The other one: ? ->A.

				If both of them are adjacent(A->B,B->A), such a transition is invalid.
				*/
				if (ResourceBarrier.Transition.StateAfter == (*iter_barrier).Transition.StateBefore)
				{
					m_ResourceBarrierHeap.erase(iter_barrier);
				}
			}

		}

		m_ResourceBarrierHeap.emplace_back(ResourceBarrier);
	}

	void CCommandList::FlushResourceBarriers()
	{
		if (m_ResourceBarrierHeap.size())
		{
			m_pCmdList->ResourceBarrier((UINT)m_ResourceBarrierHeap.size(), m_ResourceBarrierHeap.data());
			m_ResourceBarrierHeap.clear();
		}
	}
#pragma endregion

#pragma region CCommandListPool Implementation

	FrostMutexFast CCommandListPool::ms_SchedulerThreadSafeScope;

	CCommandListPool::CCommandListPool(ID3D12Device* pDevice, CCommandListFenceSet& cmdFence, int poolFenceId) :
		m_pDevice(pDevice),
		m_CmdFences(cmdFence),
		m_PoolFenceId(poolFenceId)
	{

	}

	CCommandListPool::~CCommandListPool()
	{

	}

	bool CCommandListPool::Init(D3D12_COMMAND_LIST_TYPE eType, UINT nodeMask)
	{
		if (!m_pCmdQueue)
		{
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};

			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = m_PoolType = eType;
			queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			queueDesc.NodeMask = m_nodeMask = nodeMask;

			if (FAILED(m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCmdQueue))))
			{
				DX12_ERROR("Could not create command queue");
				return false;
			}
		}

		m_AsyncCommandQueue.Init(this);

		return true;
	}

	void CCommandListPool::Clear()
	{
		// No running command lists of any type allowed:
		// * trigger Live-to-Busy transitions
		// * trigger Busy-to-Free transitions
		while (m_LiveCommandLists.size() || m_BusyCommandLists.size())
		{
			ScheduleCommandLists();
		}

		m_FreeCommandLists.clear();
		m_AsyncCommandQueue.Clear();

		m_pCmdQueue = nullptr;
	}

	/*--------------CCommandListPool functions--------------*/
	void CCommandListPool::ScheduleCommandLists()
	{
		// Remove finished command-lists from the head of the live-list
		while (m_LiveCommandLists.size())
		{
			DX12_PTR(IFrostCommandList) pICmdList = m_LiveCommandLists.front();
			CCommandList* pCmdList = static_cast<CCommandList*>(pICmdList.get());

			// free -> complete -> submitted -> finished -> clearing -> free
			if (pCmdList->IsFinishedOnGPU() && !pCmdList->IsClearing())
			{
				m_LiveCommandLists.pop_front();
				m_BusyCommandLists.push_back(pICmdList);

				pCmdList->Clear();
			}
			else
			{
				break;
			}
		}

		// Submit completed but not yet submitted command-lists from the head of the live-list
		for (int t = 0; t < m_LiveCommandLists.size(); ++t)
		{
			CCommandList* pCmdList = static_cast<CCommandList*>(m_LiveCommandLists[t].get());

			if (pCmdList->IsScheduled() && !pCmdList->IsSubmitted())
			{
				pCmdList->Submit();
			}

			if (!pCmdList->IsSubmitted())
			{
				break;
			}
		}

		// Remove cleared/deallocated command-lists from the head of the busy-list
		while (m_BusyCommandLists.size())
		{
			DX12_PTR(IFrostCommandList) pCmdList = m_BusyCommandLists.front();

			// free -> complete -> submitted -> finished -> clearing -> free
			if (static_cast<CCommandList*>(pCmdList.get())->IsFree())
			{
				m_BusyCommandLists.pop_front();
				m_FreeCommandLists.push_back(pCmdList);
			}
			else
			{
				break;
			}
		}
	}

	void CCommandListPool::CreateOrReuseCommandList(DX12_PTR(IFrostCommandList)& pICmdList)
	{
		if (m_FreeCommandLists.empty())
		{
			pICmdList = new CCommandList(*this);
		}
		else
		{
			pICmdList = m_FreeCommandLists.front();

			m_FreeCommandLists.pop_front();
		}

		static_cast<CCommandList*>(pICmdList.get())->Register();
		m_LiveCommandLists.emplace_back(pICmdList);
	}

	void CCommandListPool::AcquireCommandList(DX12_PTR(IFrostCommandList)& pICmdList) threadsafe
	{
		FrostAutoLock<FrostMutexFast> threadSafeScope(ms_SchedulerThreadSafeScope);
		{
			ScheduleCommandLists();

			CreateOrReuseCommandList(pICmdList);
		}
	}

	void CCommandListPool::ForfeitCommandList(DX12_PTR(IFrostCommandList)& pICmdList, bool bWait) threadsafe
	{
		FrostAutoLock<FrostMutexFast> threadSafeScope(ms_SchedulerThreadSafeScope);
		{
			CCommandList* pCmdList = static_cast<CCommandList*>(pICmdList.get());

			DX12_ASSERT(pCmdList->IsCompleted(), "It's not possible to forfeit an unclosed command list!");

			pCmdList->Schedule();
			pICmdList = nullptr;

			ScheduleCommandLists();

			if (bWait)
				pCmdList->WaitForFinishOnCPU();
		}
	}

	void CCommandListPool::AcquireCommandLists(UINT32 NumCLs, DX12_PTR(IFrostCommandList)* ppICmdList) threadsafe
	{
		static FrostMutexFast AcquireCLsThreadSafeScope;
		FrostAutoLock<FrostMutexFast> threadSafeScope(AcquireCLsThreadSafeScope);
		{
			ScheduleCommandLists();

			for (UINT i = 0; i < NumCLs; ++i)
				CreateOrReuseCommandList(ppICmdList[i]);
		}
	}

	void CCommandListPool::ForfeitCommandLists(UINT32 NumCLs, DX12_PTR(IFrostCommandList)* ppICmdList, bool bWait) threadsafe
	{
		static FrostMutexFast ForfeitCLsThreadSafeScope;
		FrostAutoLock<FrostMutexFast> threadSafeScope(ForfeitCLsThreadSafeScope);
		{
			CCommandList* pCmdList = static_cast<CCommandList*>(ppICmdList[NumCLs - 1].get());

			int i = NumCLs;
			while (--i >= 0)
			{
				DX12_ASSERT(static_cast<CCommandList*>(ppICmdList[i].get())->IsCompleted(),
					"It's not possible to forfeit an unclosed command list!");

				static_cast<CCommandList*>(ppICmdList[i].get())->Schedule();
				ppICmdList[i] = nullptr;
			}

			ScheduleCommandLists();

			if (bWait)
				pCmdList->WaitForFinishOnCPU();
		}

	}

	void CCommandListPool::WaitForFenceOnGPU(const UINT64 fenceValue, const int id)
	{
		if (!m_CmdFences.IsCompleted(fenceValue, id))
		{
			m_AsyncCommandQueue.Wait(m_CmdFences.GetD3D12Fence(id), fenceValue);
		}
	}

	void CCommandListPool::WaitForFenceOnGPU(const UINT64 fenceValue)
	{
		return WaitForFenceOnGPU(fenceValue, m_PoolFenceId);
	}

	void CCommandListPool::WaitForFenceOnCPU(const UINT64 fenceValue, const int id)
	{
		if (!m_CmdFences.IsCompleted(fenceValue, id))
		{
			m_CmdFences.WaitForFence(fenceValue, id);
		}
	}

	void CCommandListPool::WaitForFenceOnCPU(const UINT64 fenceValue)
	{
		return WaitForFenceOnCPU(fenceValue, m_PoolFenceId);
	}

	void CCommandListPool::SignalFenceOnGPU(const UINT64 fenceValue)
	{
		GetAsyncCommandQueue()->Signal(GetD3D12Fence(), fenceValue);
	}

	void CCommandListPool::SignalFenceOnCPU(const UINT64 fenceValue)
	{
		GetD3D12Fence()->Signal(fenceValue);
	}
#pragma endregion
}