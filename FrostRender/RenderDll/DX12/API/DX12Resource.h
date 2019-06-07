#pragma once

#include"DX12\Interface\FrostRenderDXGI.h"
#include"DX12Base.h"

namespace FrostDX12
{
	class CHeapAllocator;
	class CSwapChain;
	class CCommandList;

	class CResource : public IFrostResource
	{
	public:
		virtual void AddRef()  override;
		virtual void Release() override;

		bool MappedWriteToSubresource (
			UINT Subresource, const D3D12_RANGE* pWrittenRange, const void* pInData) override;
		bool MappedReadFromSubresource(
			UINT Subresource, const D3D12_RANGE* pReadRange,	void* pOutData)		 override;

		HRESULT STDMETHODCALLTYPE Map(
			UINT Subresource,
			_In_opt_  const D3D12_RANGE *pReadRange,
			_Outptr_opt_result_bytebuffer_(_Inexpressible_("Dependent on resource"))  void **ppData) override;

		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const override
		{
			return m_GPUVirtualAddress;
		}

		ID3D12Resource* GetD3D12Resource() const override
		{
			return m_pD3D12Resource;
		}

	public:
		CResource(ID3D12Device* pDevice, CHeapAllocator* pHeapAllocator = nullptr);
		~CResource();
	
		CResource(CResource&& r);
		CResource& operator=(CResource&& r);

		bool Init(ID3D12Resource* pResource, const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES InitialState);
		bool Init(ID3D12Resource* pResource, D3D12_RESOURCE_STATES InitialState);

		void Invalidate();

		void SetDX12SwapChain(CSwapChain* pOwner)
		{
			m_pSwapChainOwner = pOwner;
		}

		CSwapChain* GetDX12SwapChain() const
		{
			return m_pSwapChainOwner;
		}

		bool IsBackBuffer() const
		{
			return m_pSwapChainOwner != nullptr;
		}

		void VerifyBackBuffer();

		bool IsCompressed() const
		{
			return m_bCompressed;
		}

		bool IsOffCard() const
		{
			return m_HeapType == D3D12_HEAP_TYPE_READBACK || m_HeapType == D3D12_HEAP_TYPE_UPLOAD;
		}

		bool IsTarget() const
		{
			return !!(GetState() & (D3D12_RESOURCE_STATE_RENDER_TARGET | D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_DEPTH_WRITE));
		}

		bool IsGeneric() const
		{
			return !!(GetState() & (D3D12_RESOURCE_STATE_UNORDERED_ACCESS | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
		}

		bool IsGraphics() const
		{
			return !!(GetState() & (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_INDEX_BUFFER | D3D12_RESOURCE_STATE_STREAM_OUT));
		}

		const D3D12_RESOURCE_DESC& GetDesc() const
		{
			return m_ResourceDesc;
		}

		UINT64 GetSize(UINT FirstSubresource, UINT NumSubresources) const
		{
			UINT64 size;
			m_pDevice->GetCopyableFootprints(
				&m_ResourceDesc, FirstSubresource, NumSubresources,
				0, nullptr, nullptr, nullptr, &size);
			return size;
		}

		UINT32 GetPlaneCount()
		{
			return m_PlaneCount;
		}

		const NODE64& GetNodeMasks() const
		{
			return m_NodeMasks;
		}

		const D3D12_HEAP_TYPE& GetHeapType() const
		{
			return m_HeapType;
		}

		// Get current known resource barrier state
		D3D12_RESOURCE_STATES GetState() const
		{
			return m_CurrentState;
		}

		// Get announced resource barrier state
		D3D12_RESOURCE_STATES GetAnnouncedState() const
		{
			return m_AnnouncedState;
		}

		D3D12_RESOURCE_STATES GetMergedState() const
		{
			return D3D12_RESOURCE_STATES(m_CurrentState | (m_AnnouncedState != (D3D12_RESOURCE_STATES)-1 ? m_AnnouncedState : 0));
		}

		D3D12_RESOURCE_STATES GetTargetState() const
		{
			return D3D12_RESOURCE_STATES(m_AnnouncedState != (D3D12_RESOURCE_STATES)-1 ? m_AnnouncedState : m_CurrentState);
		}

		//Transition resource to desired state
		void TransitionBarrier(
			CCommandList* pCmdList,
			const D3D12_RESOURCE_STATES& DesiredState,
			UINT SubResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			const D3D12_RESOURCE_BARRIER_FLAGS& Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);

		bool NeedTransitionBarrier(const D3D12_RESOURCE_STATES& DesiredState);

		bool MatchSplitBarrier(const D3D12_RESOURCE_BARRIER_FLAGS& Flags);

		bool IsPromotableState(const D3D12_RESOURCE_STATES& DesiredState);

		bool IsCompatibleState(const D3D12_RESOURCE_STATES& DesiredState);

		void DecayTransitionBarrier(const CCommandList* commandList);

	protected:
		// Never changes after construction
		D3D12_RESOURCE_DESC		  m_ResourceDesc;
		D3D12_HEAP_TYPE			  m_HeapType;
		D3D12_GPU_VIRTUAL_ADDRESS m_GPUVirtualAddress;
		DX12_PTR(ID3D12Resource)  m_pD3D12Resource;

		// Potentially changes on every resource-use
		D3D12_RESOURCE_STATES	  m_CurrentState;
		D3D12_RESOURCE_STATES	  m_AnnouncedState;
		
		CSwapChain*  m_pSwapChainOwner;
		ID3D12Device* m_pDevice;
		
		CHeapAllocator* m_pHeapAllocator;

		FrostMutexFast  m_ResourceThreadSafeScope;
		bool m_bSplitBarrier;

		NODE64		  m_NodeMasks;
		volatile long m_RefCount;
		UINT8		  m_PlaneCount;
		bool		  m_bCompressed;
	};
}