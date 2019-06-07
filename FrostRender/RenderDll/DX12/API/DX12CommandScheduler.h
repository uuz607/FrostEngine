#pragma once

#include"DX12\Interface\FrostRender.h"
#include"DX12Base.h"
#include"DX12CommandList.h"

namespace FrostDX12
{
	class CCommandScheduler : public IRefCounted
	{
	public:
		virtual inline void AddRef() override;
		virtual inline void Release() override;

	public:
		CCommandScheduler(ID3D12Device* pDevice, UINT NodeMask);
		~CCommandScheduler();

		CCommandListPool* GetCommandListPool(int queueIndex)
		{
			return &m_CmdListPools[queueIndex];
		}

		void GetCommandList(int queueIndex, DX12_PTR(IFrostCommandList)& pCmdList);

		void WaitForFinishOnGPU(int queueIndex, IFrostCommandList* pICmdList);

		void ExcuteCommandList(DX12_PTR(IFrostCommandList)& pCmdList, bool bWait = false);

		void RecreateCommandListPool(int queueIndex, UINT NodeMask);

		// Fence management API
		const CCommandListFenceSet& GetFenceManager() const { return m_CmdFenceSet; }

	private:
		CCommandListFenceSet m_CmdFenceSet;
#ifdef _ALLOW_INITIALIZER_LISTS
		CCommandListPool	m_CmdListPools[CMDQUEUE_NUM];
#else
		std::vector<CCommandListPool> m_CmdListPools;
#endif 

		volatile long m_RefCount;
	};
}