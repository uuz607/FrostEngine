
#include"pch.h"
#include"DX12CommandScheduler.h"

namespace FrostDX12
{
	/*--------------Interface implemtation--------------*/
	void CCommandScheduler::AddRef()
	{
		::_InterlockedIncrement(&m_RefCount);
	}

	void CCommandScheduler::Release()
	{
		long refCount = ::_InterlockedDecrement(&m_RefCount);
		if (!refCount)
		{
			delete this;
		}
	}

	/*--------------CCommandScheduler implemtation--------------*/
	CCommandScheduler::CCommandScheduler(ID3D12Device* pDevice, UINT NodeMask) :
		m_CmdFenceSet(pDevice),
#ifdef _ALLOW_INITIALIZER_LISTS
		m_CmdListPools
	{
		{ pDevice, m_CmdFenceSet, CMDQUEUE_GRAPHICS },
		{ pDevice, m_CmdFenceSet, CMDQUEUE_COMPUTE },
		{ pDevice, m_CmdFenceSet, CMDQUEUE_COPY }
	},
#endif
		m_RefCount(0)
	{
#ifndef _ALLOW_INITIALIZER_LISTS
		m_CmdListPools.emplace_back(pDevice, m_CmdFenceSet, CMDQUEUE_GRAPHICS);
		m_CmdListPools.emplace_back(pDevice, m_CmdFenceSet, CMDQUEUE_COMPUTE);
		m_CmdListPools.emplace_back(pDevice, m_CmdFenceSet, CMDQUEUE_COPY);
#endif 

		m_CmdListPools[CMDQUEUE_GRAPHICS].Init(CMDLIST_GRAPHICS, NodeMask);
		m_CmdListPools[CMDQUEUE_COMPUTE].Init(CMDLIST_COMPUTE, NodeMask);
		m_CmdListPools[CMDQUEUE_COPY].Init(CMDLIST_COPY, NodeMask);

		m_CmdFenceSet.Init();
	}


	CCommandScheduler::~CCommandScheduler()
	{

	}

	/*--------------Get() & Set() & Is()--------------*/
	void CCommandScheduler::GetCommandList(int queueIndex, DX12_PTR(IFrostCommandList)& pCmdList)
	{
		m_CmdListPools[queueIndex].AcquireCommandList(pCmdList);
	}

	void CCommandScheduler::ExcuteCommandList(DX12_PTR(IFrostCommandList)& pCmdList, bool bWait)
	{
		static_cast<CCommandList*>(pCmdList.get())->End();
		static_cast<CCommandList*>(pCmdList.get())->GetCommandListPool().ForfeitCommandList(pCmdList,bWait);
	}

	void CCommandScheduler::WaitForFinishOnGPU(int queueIndex, IFrostCommandList* pICmdList)
	{
		m_CmdListPools[queueIndex].WaitForFenceOnGPU(
			static_cast<CCommandList*>(pICmdList)->GetCurrentFenceValue(),
			static_cast<CCommandList*>(pICmdList)->GetCommandListPool().GetFenceID());
	}

	/*--------------CCommandScheduler functions--------------*/
	void CCommandScheduler::RecreateCommandListPool(int queueIndex, UINT NodeMask)
	{
		m_CmdListPools[queueIndex].Clear();
		m_CmdListPools[queueIndex].Init(s_QueueTypeToCmdlistType[queueIndex], NodeMask);
	}
}