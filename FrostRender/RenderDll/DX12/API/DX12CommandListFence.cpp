
#include"pch.h"
#include"DX12CommandListFence.h"
#include"DX12Device.h"

namespace FrostDX12
{
#pragma region CCommandListFence implementation

	CCommandListFence::CCommandListFence(ID3D12Device* pDevice) :
		 m_pDevice(pDevice),
		 m_CurrentValue(0),
		 m_LastCompletedValue(0)
	{

	}

	CCommandListFence::~CCommandListFence()
	{
		m_pFence->Release();
		CloseHandle(m_FenceEvent);
	}

	bool CCommandListFence::Init()
	{
		if (FAILED(m_pDevice->CreateFence(0U, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence))))
		{
			DX12_ERROR("Could not create fence object!");
			return false;
		}

		m_pFence->Signal(m_LastCompletedValue);
		m_FenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);

		return true;
	}

	/*---------------CCommandListFence functions---------------*/
	UINT64 CCommandListFence::AdvanceCompletion() threadsafe_const
	{
		// Check current completed fence
		UINT64 currentCompletedValue = m_pFence->GetCompletedValue();

		// CLs may terminate in any order. Is it higher than last known completed fence? If so, update it!
		MaxFenceValue(m_LastCompletedValue, currentCompletedValue);

		return currentCompletedValue;
	}

	void CCommandListFence::WaitForFence(UINT64 fenceValue)
	{
		if (!IsCompleted(fenceValue))
		{
			m_pFence->SetEventOnCompletion(fenceValue, m_FenceEvent);
			WaitForSingleObject(m_FenceEvent, INFINITE);

			AdvanceCompletion();
		}
	}
#pragma endregion

#pragma region CComandListFenceSet implementation
	
	CCommandListFenceSet::CCommandListFenceSet(ID3D12Device* pDevice) :
		m_pDevice(pDevice)
	{
		m_CurrentValues[CMDQUEUE_GRAPHICS] = m_CurrentValues[CMDQUEUE_COMPUTE] = m_CurrentValues[CMDQUEUE_COPY] = 0;
		m_SubmittedValues[CMDQUEUE_GRAPHICS] = m_SubmittedValues[CMDQUEUE_COMPUTE] = m_SubmittedValues[CMDQUEUE_COPY] = 0;
		m_LastCompletedValues[CMDQUEUE_GRAPHICS] = m_LastCompletedValues[CMDQUEUE_COMPUTE] = m_LastCompletedValues[CMDQUEUE_COPY] = 0;
	}

	CCommandListFenceSet::~CCommandListFenceSet()
	{
		for (int i = 0; i < CMDQUEUE_NUM; ++i)
		{
			m_pFences[i]->Release();

			CloseHandle(m_FenceEvents[i]);
		}
	}

	bool CCommandListFenceSet::Init()
	{
		for (int i = 0; i < CMDQUEUE_NUM; ++i)
		{
			if (FAILED(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFences[i]))))
			{
				DX12_ERROR("Could not create fence object!");
				return false;
			}

			m_pFences[i]->Signal(m_LastCompletedValues[i]);
			m_FenceEvents[i] = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		}

		return true;
	}

	/*---------------CCommandListFenceSet functions---------------*/
	void CCommandListFenceSet::WaitForFence(const UINT64 fenceValue, const int id) const
	{
		m_pFences[id]->SetEventOnCompletion(fenceValue, m_FenceEvents[id]);
		WaitForSingleObject(m_FenceEvents[id], INFINITE);
	}

	void CCommandListFenceSet::WaitForFence(const UINT64(&fenceValues)[CMDQUEUE_NUM]) const
	{
		// NOTE: event does ONLY trigger when the value has been set (it'll lock when trying with 0)
		int numEvents = 0;
		for (int id = 0; id < CMDQUEUE_NUM; ++id)
		{
			if (fenceValues[id] && (m_LastCompletedValues[id] < fenceValues[id]))
			{
				m_pFences[id]->SetEventOnCompletion(fenceValues[id], m_FenceEvents[numEvents++]);
			}
		}

		if (numEvents)
			WaitForMultipleObjects(numEvents, m_FenceEvents, TRUE, INFINITE);

		AdvanceCompletion();
	}
#pragma endregion

}