#pragma once

#include<atomic>
#include"DX12\Interface\FrostRender.h"
#include"DX12Base.h"

namespace FrostDX12
{	

	typedef std::atomic<UINT64> FVAL64;

	inline void upr(UINT64& a, const UINT64 b)
	{
		// Generate conditional moves instead of branches
		a = (a >= b ? a : b);
	}

	inline UINT64 MaxFenceValue(FVAL64& a, const UINT64& b)
	{
		UINT64 utilizedValue = b;
		UINT64 previousValue = a;
		while (previousValue < utilizedValue &&
			!a.compare_exchange_weak(previousValue, utilizedValue)) {};

		return previousValue;
	}

	class CCommandListFence
	{
	public:
		CCommandListFence(ID3D12Device* pDevice);
		~CCommandListFence();

		bool Init();

		ID3D12Fence* GetFence() threadsafe_const
		{
			return m_pFence;
		}

		UINT64 GetCurrentValue() threadsafe_const
		{
			return m_CurrentValue;
		}

		void SetCurrentValue(UINT64 fenceValue) threadsafe
		{
			// CLs may submit in any order. Is it higher than last known completed fence? If so, update it!
			MaxFenceValue(m_CurrentValue, fenceValue);
		}

		bool IsCompleted(UINT64 fenceValue) threadsafe_const
		{
			// Check against last known completed value first to avoid unnecessary fence read
			return(m_LastCompletedValue >= fenceValue) || (AdvanceCompletion() >= fenceValue);
		}

		UINT64 GetLastCompletedFenceValue() threadsafe_const
		{
			return m_LastCompletedValue;
		}

		void WaitForFence(UINT64 fenceValue) threadsafe;

		UINT64 AdvanceCompletion() threadsafe_const;

	private:
		ID3D12Device*  m_pDevice;
		ID3D12Fence*   m_pFence;
		HANDLE         m_FenceEvent;

		FVAL64         m_CurrentValue;
		mutable FVAL64 m_LastCompletedValue;
	};

	class CCommandListFenceSet
	{
	public:
		CCommandListFenceSet(ID3D12Device* pDevice);
		~CCommandListFenceSet();

		bool   Init();

		ID3D12Fence** GetD3D12Fences() threadsafe
		{
			return m_pFences;
		}

		ID3D12Fence* GetD3D12Fence(const int id) threadsafe_const
		{
			return m_pFences[id];
		}

		UINT64 GetSubmittedValue(const int id) threadsafe_const
		{
			return m_SubmittedValues[id];
		}

		void SetSubmittedValue(const UINT64 fenceValue, const int id) threadsafe
		{
			// CLs may submit in any order. Is it higher than last known submitted fence? If so, update it!
			MaxFenceValue(m_SubmittedValues[id], fenceValue);
		}

		UINT64 GetCurrentValue(const int id) threadsafe_const
		{
			return m_CurrentValues[id];
		}

		void GetCurrentValues(UINT64(&fenceValues)[CMDQUEUE_NUM]) threadsafe_const
		{
			fenceValues[CMDQUEUE_GRAPHICS] = m_CurrentValues[CMDQUEUE_GRAPHICS];
			fenceValues[CMDQUEUE_COMPUTE] = m_CurrentValues[CMDQUEUE_COMPUTE];
			fenceValues[CMDQUEUE_COPY] = m_CurrentValues[CMDQUEUE_COPY];
		}

		const FVAL64(&GetCurrentValues() threadsafe_const)[CMDQUEUE_NUM]
		{
			return m_CurrentValues;
		}

		void SetCurrentValue(
			const UINT64 fenceValue,
			const int id) threadsafe
		{
			// CLs may submit in any order. Is it higher than last known completed fence? If so, update it!
			MaxFenceValue(m_CurrentValues[id], fenceValue);
		}

		bool IsCompleted(
			const UINT64 fenceValue,
			const int id) threadsafe_const
		{
			// Check against last known completed value first to avoid unnecessary fence read
			return (m_LastCompletedValues[id] >= fenceValue) || (AdvanceCompletion(id) >= fenceValue);
		}

		bool IsCompleted(const UINT64(&fenceValues)[CMDQUEUE_NUM]) threadsafe_const
		{
			// Check against last known completed value first to avoid unnecessary fence read
			return
				// TODO: return mask of completed fences so we don't check all three of them all the time
				((m_LastCompletedValues[CMDQUEUE_GRAPHICS] >= fenceValues[CMDQUEUE_GRAPHICS]) || (AdvanceCompletion(CMDQUEUE_GRAPHICS) >= fenceValues[CMDQUEUE_GRAPHICS])) &
				((m_LastCompletedValues[CMDQUEUE_COMPUTE] >= fenceValues[CMDQUEUE_COMPUTE]) || (AdvanceCompletion(CMDQUEUE_COMPUTE) >= fenceValues[CMDQUEUE_COMPUTE]))  &
				((m_LastCompletedValues[CMDQUEUE_COPY] >= fenceValues[CMDQUEUE_COPY]) || (AdvanceCompletion(CMDQUEUE_COPY) >= fenceValues[CMDQUEUE_COPY]));
		}

		UINT64 GetLastCompletedFenceValue(const int id) threadsafe_const
		{
			return m_LastCompletedValues[id];
		}

		void GetLastCompletedFenceValues(UINT64(&fenceValues)[CMDQUEUE_NUM]) threadsafe_const
		{
			fenceValues[CMDQUEUE_GRAPHICS] = m_LastCompletedValues[CMDQUEUE_GRAPHICS];
			fenceValues[CMDQUEUE_COMPUTE] = m_LastCompletedValues[CMDQUEUE_COMPUTE];
			fenceValues[CMDQUEUE_COPY] = m_LastCompletedValues[CMDQUEUE_COPY];
		}

		const FVAL64(&GetLastCompletedFenceValues() threadsafe_const)[CMDQUEUE_NUM]
		{
			return m_LastCompletedValues;
		}

		void WaitForFence(const UINT64 fenceValue, const int id)	threadsafe_const;
		void WaitForFence(const UINT64(&fenceValues)[CMDQUEUE_NUM]) threadsafe_const;

		UINT64 AdvanceCompletion(const int id) threadsafe_const
		{
			// Check current completed fence
			UINT64 currentCompletedValue = m_pFences[id]->GetCompletedValue();

			// CLs may terminate in any order. Is it higher than last known completed fence? If so, update it!
			MaxFenceValue(m_LastCompletedValues[id], currentCompletedValue);

			return currentCompletedValue;
		}

		void AdvanceCompletion() threadsafe_const
		{
			AdvanceCompletion(CMDQUEUE_GRAPHICS);
			AdvanceCompletion(CMDQUEUE_COMPUTE);
			AdvanceCompletion(CMDQUEUE_COPY);
		}

	private:
		ID3D12Device* m_pDevice;
		ID3D12Fence*  m_pFences[CMDQUEUE_NUM];
		HANDLE        m_FenceEvents[CMDQUEUE_NUM];

		// Maximum fence-value of all command-lists currently in flight (allocated, running or free)
		FVAL64 m_CurrentValues[CMDQUEUE_NUM];
		FVAL64 m_SubmittedValues[CMDQUEUE_NUM];

		// Maximum fence-value of all command-lists executed by the driver (free only)
		mutable FVAL64 m_LastCompletedValues[CMDQUEUE_NUM];
	};

}

