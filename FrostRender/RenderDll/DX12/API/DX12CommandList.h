#pragma once

#include"DX12\Interface\FrostRender.h"
#include"DX12Base.h"
#include"DX12CommandListFence.h"
#include"DX12PSO.h"
#include"DX12DescriptorHeap.h"
#include"DX12View.h"
#include"DX12SamplerState.h"
#include"DX12AsynCommandQueue.h"


#define CMDLIST_TYPE_DIRECT   0 // D3D12_COMMAND_LIST_TYPE_DIRECT
#define CMDLIST_TYPE_BUNDLE   1 // D3D12_COMMAND_LIST_TYPE_BUNDLE
#define CMDLIST_TYPE_COMPUTE  2 // D3D12_COMMAND_LIST_TYPE_COMPUTE
#define CMDLIST_TYPE_COPY     3 // D3D12_COMMAND_LIST_TYPE_COPY

#define CMDLIST_DEFAULT		  D3D12_COMMAND_LIST_TYPE_DIRECT
#define CMDLIST_GRAPHICS      FrostDX12::s_QueueTypeToCmdlistType[CMDQUEUE_GRAPHICS]
#define CMDLIST_COMPUTE       FrostDX12::s_QueueTypeToCmdlistType[CMDQUEUE_COMPUTE]
#define CMDLIST_COPY          FrostDX12::s_QueueTypeToCmdlistType[CMDQUEUE_COPY]


#define CMDQUEUE_GRAPHICS_IOE true  // graphics queue will terminate in-order
#define CMDQUEUE_COMPUTE_IOE  false // compute queue can terminate out-of-order
#define CMDQUEUE_COPY_IOE     false // copy queue can terminate out-of-order

#define DX12_BARRIER_FUSION   true

#define COMMAND_LIST_OPERATION_CONFIGURE 0
#define COMMAND_LIST_OPERATION_RESOURCE  1

namespace FrostDX12
{
	static D3D12_COMMAND_LIST_TYPE s_QueueTypeToCmdlistType[CMDQUEUE_NUM] =
	{
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		D3D12_COMMAND_LIST_TYPE_COMPUTE,
		D3D12_COMMAND_LIST_TYPE_COPY
	};

	class CView;

	class CCommandListPool
	{
	public:
		CCommandListPool(ID3D12Device* pDevice, CCommandListFenceSet& fences, int poolFenceId);
		~CCommandListPool();

		bool Init(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE(CMDLIST_DEFAULT), UINT nodeMask = 0);
		void Clear();

#pragma region Set() & Get() & Is()

		int GetFenceID()
		{
			return m_PoolFenceId;
		}

		UINT GetNodeMask()
		{
			return m_nodeMask;
		}

		ID3D12Device* GetDevice()
		{
			return m_pDevice;
		}

		ID3D12Fence* GetD3D12Fence()
		{
			return m_CmdFences.GetD3D12Fence(m_PoolFenceId);
		}

		ID3D12Fence** GetD3D12Fences()
		{
			return m_CmdFences.GetD3D12Fences();
		}

		CCommandListFenceSet& GetFences()
		{
			return m_CmdFences;
		}

		ID3D12CommandQueue* GetD3D12CommandQueue()
		{
			return m_pCmdQueue;
		}

		D3D12_COMMAND_LIST_TYPE GetD3D12QueueType()
		{
			return m_PoolType;
		}

		CAsyncCommandQueue* GetAsyncCommandQueue()
		{
			return &m_AsyncCommandQueue;
		}

		UINT64 GetCurrentFenceValue()
		{
			return m_CmdFences.GetCurrentValue(m_PoolFenceId);
		}

		void SetCurrentFenceValue(const UINT64 fenceValue)
		{
			m_CmdFences.SetCurrentValue(fenceValue, m_PoolFenceId);
		}

		UINT64 GetLastCompletedFenceValue()
		{
			m_CmdFences.GetLastCompletedFenceValue(m_PoolFenceId);
		}

		bool IsCompletedPerspCPU(const UINT64 fenceValue, const int id)
		{
			return m_CmdFences.IsCompleted(fenceValue, id);
		}

		bool IsCompletedPerspCPU(const UINT64 fenceValue)
		{
			return m_CmdFences.IsCompleted(fenceValue, m_PoolFenceId);
		}
#pragma endregion

		void AcquireCommandList(DX12_PTR(IFrostCommandList)& pICmdList);
		void ForfeitCommandList(DX12_PTR(IFrostCommandList)& pICmdList, bool bWait = false);
		void AcquireCommandLists(UINT32 numCLs, DX12_PTR(IFrostCommandList)* ppICmdList);
		void ForfeitCommandLists(UINT32 numCLs, DX12_PTR(IFrostCommandList)* ppICmdList, bool bWait = false);

		void WaitForFenceOnGPU(const UINT64 fenceValue, const int id);
		void WaitForFenceOnGPU(const UINT64 fenceValue);

		void WaitForFenceOnCPU(const UINT64 fenceValue, const int id);
		void WaitForFenceOnCPU(const UINT64 fenceValue);

		void SignalFenceOnGPU(const UINT64 fenceValue);
		void SignalFenceOnCPU(const UINT64 fenceValue);

	private:
		void ScheduleCommandLists();
		void CreateOrReuseCommandList(DX12_PTR(IFrostCommandList)& result);

		static FrostMutexFast ms_SchedulerThreadSafeScope;

		ID3D12Device*			m_pDevice;
		CCommandListFenceSet&   m_CmdFences;
		D3D12_COMMAND_LIST_TYPE m_PoolType;

		std::deque<DX12_PTR(IFrostCommandList)> m_LiveCommandLists;
		std::deque<DX12_PTR(IFrostCommandList)> m_BusyCommandLists;
		std::deque<DX12_PTR(IFrostCommandList)> m_FreeCommandLists;

		DX12_PTR(ID3D12CommandQueue) m_pCmdQueue;
		CAsyncCommandQueue			 m_AsyncCommandQueue;
		UINT                         m_nodeMask;
		int							 m_PoolFenceId;
	};

	class CCommandList : public IFrostCommandList
	{
	public:
		virtual void AddRef() override;

		virtual void Release() override;

		UINT64 GetCurrentFenceValue() const override
		{
			return m_CurrentFenceValue;
		}

		UINT64 SignalFenceOnGPU() override;

		UINT64 SignalFenceOnCPU() override;

		void WaitForFinishOnGPU(
			const UINT64 FenceValue,
			const int id) const override;

		void WaitForFinishOnCPU() const override;

		void ResourceBarrier(
			IFrostResource* pResource,
			const D3D12_RESOURCE_STATES& DesiredState,
			UINT SubResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			const D3D12_RESOURCE_BARRIER_FLAGS& Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) override;

		void AliasBarrier(
			IFrostResource* pResourceBefore,
			IFrostResource* pResourceAfter) override;

		void SetVertexBuffers(
			UINT StartSlot,
			UINT NumViews,
			IFrostView** ppViews) override;

		void SetIndexBuffer(IFrostView* pView) override;

		void SetPipelineState(IFrostPSO* pso)  override;

		void SetGraphicsRootSignature(IFrostRootSignature* pRootSignature) override;

		void SetComputeRootSignature(IFrostRootSignature* pRootSignature)  override;

		void SetGraphicsDescriptorTable(
			UINT RootParameterIndex,
			IFrostView* pView) override;

		void SetGraphicsDescriptorTable(
			UINT RootParameterIndex,
			IFrostSamplerState* pSampler) override;

		void SetRenderTargets(
			UINT NumRenderTargetDescriptors,
			IFrostView** pRenderTargets,
			BOOL RTsSingleHandleToDescriptorRange,
			IFrostView* pDepthStencil) override;

		void SetStencilRef(UINT StencilRef) override;

		void SetViewports(UINT NumViewports, const D3D12_VIEWPORT* pViewports) override;

		void SetScissorRects(UINT NumRects, const D3D12_RECT* pRects) override;

		void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology) override;

		void SetDescriptorHeaps(IFrostDescriptorAllocator* heaps) override;

		void ClearDepthStencilView(
			IFrostView* pDepthStencilView,
			D3D12_CLEAR_FLAGS ClearFlags,
			float depthValue,
			UINT StencilValue,
			UINT NumRects = 0U,
			const D3D12_RECT* pRect = nullptr)  override;

		void ClearRenderTargetView(
			IFrostView* pRenderTargetView,
			const FLOAT rgba[4],
			UINT NumRects = 0U,
			const D3D12_RECT* pRects = nullptr) override;

		void ClearUnorderedAccessView(
			IFrostView* pUnorderedAccessView,
			const UINT  rgba[4],
			UINT NumRects = 0U,
			const D3D12_RECT* pRects = nullptr) override;

		void ClearUnorderedAccessView(
			IFrostView* pUnorderedAccessView,
			const FLOAT rgba[4],
			UINT NumRects = 0U,
			const D3D12_RECT* pRects = nullptr) override;

		void DrawInstanced(
			UINT VertexCountPerInstance,
			UINT InstanceCount,
			UINT StartVertexLocation,
			UINT StartInstanceLocation) override;

		void DrawIndexedInstanced(
			UINT IndexCountPerInstance,
			UINT InstanceCount,
			UINT StartIndexLocation,
			UINT BaseVertexLocation,
			UINT StartInstanceLocation) override;

		void UpdateSubresources(
			IFrostResource* pFrostDestResource,
			IFrostResource* pFrostIntermedia,
			UINT64 IntermediateOffset,
			UINT FirstSubresource,
			UINT NumSubResources,
			D3D12_SUBRESOURCE_DATA* pSrcData)   override;

	public:
		friend class CCommandListPool;

		virtual ~CCommandList();

		bool Init(UINT64 CurrentFenceValue);

		bool Reset();

#pragma region Get() & Set() &Is()

		ID3D12GraphicsCommandList* GetD3D12CommandList() const
		{
			return m_pCmdList;
		}

		D3D12_COMMAND_LIST_TYPE GetD3D12ListType() const
		{
			return m_ListType;
		}

		ID3D12CommandAllocator* GetD3D12CommandAllocator() const
		{
			return m_pCmdAllocator;
		}

		ID3D12CommandQueue* GetD3D12CommandQueue() const
		{
			return m_pCmdQueue;
		}

		CCommandListPool& GetCommandListPool() const
		{
			return m_Pool;
		}

		bool IsFree() const
		{
			return m_State == COMMAND_LIST_STATE_FREE;
		}

		bool IsCompleted() const
		{
			return m_State >= COMMAND_LIST_STATE_COMPLETED;
		}

		bool IsUtilized() const
		{
			return m_nCommands > 0;
		}

		bool IsScheduled() const
		{
			return m_State == COMMAND_LIST_STATE_SCHEDULED;
		}

		bool IsSubmitted() const
		{
			return m_State >= COMMAND_LIST_STATE_SUBMITTED;
		}

		bool IsFinishedOnGPU() const
		{
			if ((m_State == COMMAND_LIST_STATE_SUBMITTED) && m_Pool.IsCompletedPerspCPU(m_CurrentFenceValue))
			{
				m_State = COMMAND_LIST_STATE_FINISHED;
			}

			return m_State == COMMAND_LIST_STATE_FINISHED;
		}

		bool IsClearing() const
		{
			return m_State == COMMAND_LIST_STATE_CLEARING;
		}

#pragma endregion

		void Register();

		void Schedule();

		void Clear();

		void End();

		void Submit();

		void ResourceBarrier(D3D12_RESOURCE_BARRIER& ResourceBarrier);

		void FlushResourceBarriers();

	protected:
		CCommandList(CCommandListPool& pool);

		CCommandListPool& m_Pool;

		ID3D12Device* m_pDevice;

		DX12_PTR(ID3D12CommandQueue)		m_pCmdQueue;
		DX12_PTR(ID3D12CommandAllocator)	m_pCmdAllocator;
		DX12_PTR(ID3D12GraphicsCommandList) m_pCmdList;

		D3D12_COMMAND_LIST_TYPE m_ListType;
		UINT64 m_CurrentFenceValue;

		typedef std::vector<D3D12_RESOURCE_BARRIER> ResourceBarriers;
		ResourceBarriers m_ResourceBarrierHeap;

		std::unordered_set<CResource*> m_ResourceSet;

		mutable enum
		{
			COMMAND_LIST_STATE_FREE,
			COMMAND_LIST_STATE_STARTED,
			COMMAND_LIST_STATE_UTILIZED,
			COMMAND_LIST_STATE_COMPLETED,
			COMMAND_LIST_STATE_SCHEDULED,
			COMMAND_LIST_STATE_SUBMITTED,
			COMMAND_LIST_STATE_FINISHED,
			COMMAND_LIST_STATE_CLEARING
		}m_State;

		UINT m_nodeMask;

		UINT m_nCommands;

		volatile long m_RefCount;
	};
}