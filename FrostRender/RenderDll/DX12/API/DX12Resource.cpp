#include"pch.h"
#include"DX12Resource.h"
#include"DX12Heap.h"
#include"DX12SwapChain.h"

#define DX12_BARRIER_RELAXATION   // READ-barriers are not changed to more specific subsets like INDEX_BUFFER

namespace FrostDX12
{
	/*--------------Interface implemtation--------------*/
	void CResource::AddRef()
	{
		::_InterlockedIncrement(&m_RefCount);
	}

	void CResource::Release()
	{
		long refCount = ::_InterlockedIncrement(&m_RefCount);
		if (!refCount)
		{
			delete this;
		}
	}

	bool CResource::MappedWriteToSubresource(UINT Subresource, const D3D12_RANGE* pWrittenRange, const void* pInData)
	{
		// It is valid to specify the CPU won't read any data by passing a range where End is less than or equal to Begin
		const D3D12_RANGE noRead = { 0,0 };

		void* pOutData;
		GetD3D12Resource()->Map(Subresource, &noRead, &pOutData);
		::memcpy(pOutData, pInData, pWrittenRange->End - pWrittenRange->Begin);
		GetD3D12Resource()->Unmap(Subresource, pWrittenRange);

		return true;
	}

	bool CResource::MappedReadFromSubresource(UINT Subresource, const D3D12_RANGE* pReadRange, void* pOutData)
	{
		// It is valid to specify the CPU won't read any data by passing a range where End is less than or equal to Begin
		const D3D12_RANGE noWrite = { 0, 0 };

		void* pInData;
		GetD3D12Resource()->Map(Subresource, pReadRange, &pInData);
		::memcpy(pOutData, pInData, pReadRange->End - pReadRange->Begin);
		GetD3D12Resource()->Unmap(Subresource, &noWrite);

		return true;
	}

	HRESULT STDMETHODCALLTYPE CResource::Map(
		UINT Subresource,
		_In_opt_  const D3D12_RANGE *pReadRange,
		_Outptr_opt_result_bytebuffer_(_Inexpressible_("Dependent on resource"))  void **ppData)
	{
		return GetD3D12Resource()->Map(Subresource, pReadRange, ppData);
	}

	/*--------------CResource implemtation--------------*/
	CResource::CResource(ID3D12Device* pDevice, CHeapAllocator* pHeapAllocator) :
		m_HeapType(D3D12_HEAP_TYPE_DEFAULT),
		m_GPUVirtualAddress(0ULL),
		m_pD3D12Resource(nullptr),
		m_CurrentState(D3D12_RESOURCE_STATE_COMMON),
		m_AnnouncedState(D3D12_RESOURCE_STATE_COMMON),
		m_pSwapChainOwner(nullptr),
		m_pDevice(pDevice),
		m_pHeapAllocator(pHeapAllocator),
		m_bSplitBarrier(false),
		m_RefCount(0),
		m_PlaneCount(0),
		m_bCompressed(false)
	{

	}

	CResource::CResource(CResource&& r) :
		m_HeapType(std::move(r.m_HeapType)),
		m_GPUVirtualAddress(std::move(r.m_GPUVirtualAddress)),
		m_pD3D12Resource(std::move(r.m_pD3D12Resource)),
		m_CurrentState(std::move(r.m_CurrentState)),
		m_AnnouncedState(std::move(r.m_AnnouncedState)),
		m_pSwapChainOwner(std::move(r.m_pSwapChainOwner)),
		m_pDevice(std::move(r.m_pDevice)),
		m_pHeapAllocator(std::move(r.m_pHeapAllocator)),
		m_bSplitBarrier(std::move(r.m_bSplitBarrier)),
		m_RefCount(std::move(r.m_RefCount)),
		m_PlaneCount(std::move(r.m_PlaneCount)),
		m_bCompressed(std::move(r.m_bCompressed))
	{
		r.m_pD3D12Resource = nullptr;
		r.m_pSwapChainOwner = nullptr;
		r.m_pDevice = nullptr;
	}

	CResource& CResource::operator=(CResource&& r)
	{
		m_HeapType = std::move(r.m_HeapType);
		m_GPUVirtualAddress = std::move(r.m_GPUVirtualAddress);
		m_pD3D12Resource = std::move(r.m_pD3D12Resource);
		m_CurrentState = std::move(r.m_CurrentState);
		m_AnnouncedState = std::move(r.m_AnnouncedState);
		m_pSwapChainOwner = std::move(r.m_pSwapChainOwner);
		m_pDevice = std::move(r.m_pDevice);
		m_pHeapAllocator = std::move(r.m_pHeapAllocator);
		m_bSplitBarrier = std::move(r.m_bSplitBarrier);
		m_RefCount = std::move(r.m_RefCount);
		m_PlaneCount = std::move(r.m_PlaneCount);
		m_bCompressed = std::move(r.m_bCompressed);

		r.m_pD3D12Resource = nullptr;
		r.m_pSwapChainOwner = nullptr;
		r.m_pDevice = nullptr;

		return *this;
	}

	CResource::~CResource()
	{
		if (m_pHeapAllocator)
		{
			Invalidate();
		}	
	}

	bool CResource::Init(ID3D12Resource* pResource, const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES InitialState)
	{
		m_pD3D12Resource = pResource;
		m_ResourceDesc = desc;
		m_PlaneCount = D3D12GetFormatPlaneCount(m_pDevice, m_ResourceDesc.Format);

		::memset(&m_NodeMasks, 0, sizeof(m_NodeMasks));

		if (m_pD3D12Resource)
		{
			D3D12_HEAP_PROPERTIES heapProperties;

			if (SUCCEEDED(m_pD3D12Resource->GetHeapProperties(&heapProperties, nullptr)))
			{
				m_HeapType = heapProperties.Type;
				m_CurrentState = InitialState;

				m_NodeMasks.creationMask = heapProperties.CreationNodeMask;
				m_NodeMasks.visibilityMask = heapProperties.VisibleNodeMask;
			}

			if (m_NodeMasks.creationMask == m_NodeMasks.visibilityMask)
			{
				if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
				{
					m_GPUVirtualAddress = m_pD3D12Resource->GetGPUVirtualAddress();
				}
			}
		}
		else
		{
			// Null resource put on the UPLOAD heap to prevent attempts to transition the resource.
			m_HeapType = D3D12_HEAP_TYPE_UPLOAD;
			m_CurrentState = InitialState;
		}

		/* 
			Certain heaps are restricted to certain D3D12_RESOURCE_STATES states, and cannot be changed.
			D3D12_HEAP_TYPE_UPLOAD requires D3D12_RESOURCE_STATE_GENERIC_READ.
		*/ 
		if (m_HeapType == D3D12_HEAP_TYPE_UPLOAD)
		{
			m_CurrentState = D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		else if (m_HeapType == D3D12_HEAP_TYPE_READBACK)
		{
			m_CurrentState = D3D12_RESOURCE_STATE_COPY_DEST;
		}

		m_pSwapChainOwner = nullptr;
		m_bCompressed = IsDXGIFormatCompressed(desc.Format);

		return true;
	}

	bool  CResource::Init(ID3D12Resource* pResource, D3D12_RESOURCE_STATES InitialState)
	{
		return Init(pResource, pResource->GetDesc(), InitialState);
	}

	bool CResource::IsPromotableState(const D3D12_RESOURCE_STATES& DesiredState)
	{
		/*
			Resources can only be "promoted" out of D3D12_RESOURCE_STATE_COMMON.

			Depth-stencil resources must be non-simultaneous-access textures 
			and thus can never be implicitly promoted.
		*/
		if (D3D12_RESOURCE_STATE_COMMON != m_CurrentState || 
			DesiredState & (D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_DEPTH_READ))
		{
			return false;
		}
		/*
			All buffer resources as well as textures with the D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS flag set 
			are implicitly promoted from D3D12_RESOURCE_STATE_COMMON to the relevant state on first GPU access.

			The non-simultaneous-access textures are implicitly promoted from D3D12_RESOURCE_STATE_COMMON to 
				，D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE(0x40), or
				，D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE(0x80), or
				，D3D12_RESOURCE_STATE_COPY_DEST(0x400), or 
				，D3D12_RESOURCE_STATE_COPY_SOURCE(0x800).
		*/
		switch (m_ResourceDesc.Dimension)
		{
		case D3D12_RESOURCE_DIMENSION_UNKNOWN:
			return false;

		case D3D12_RESOURCE_DIMENSION_BUFFER:
			m_CurrentState = DesiredState;
			return true;

		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			if (m_ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS)
			{
				m_CurrentState = DesiredState;
				return true;
			}
			else if (DesiredState & (0x40 | 0x80 | 0x400 | 0x800))
			{
				m_CurrentState = DesiredState;
				return true;
			}
			else
			{
				return false;
			}
			break;
		default:
			return false;
		}
	}

	bool CResource::IsCompatibleState(const D3D12_RESOURCE_STATES& DesiredState)
	{
#ifdef DX12_BARRIER_RELAXATION
		return m_CurrentState == DesiredState || m_CurrentState & DesiredState;
#else
		return m_CurrentState == DesiredState;
#endif
	}

	void CResource::VerifyBackBuffer()
	{
		DX12_ASSERT((!IsBackBuffer() || !GetDX12SwapChain()->IsPresentScheduled()),
			"Flush didn't dry out all outstanding Present() calls!");

		DX12_ASSERT((!IsBackBuffer() || GetD3D12Resource() == GetDX12SwapChain()->GetCurrentBackBuffer().GetD3D12Resource()),
			"Resource is referring to old swapchain index!");
	}

	/*--------------Utility functions--------------*/
	void CResource::DecayTransitionBarrier(const CCommandList* commandList)
	{
		/*
			The following resources will decay when an ExecuteCommandLists operation is completed on the GPU:
				，Resources being accessed on a Copy queue, or
				，Buffer resources on any queue type, or
				，Texture resources on any queue type that have the D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS flag set, or
				，Any resource implicitly promoted to a read-only state.
		*/

		if (commandList->GetD3D12ListType() == D3D12_COMMAND_LIST_TYPE_COPY)
		{
			m_AnnouncedState = D3D12_RESOURCE_STATE_COMMON;
			m_CurrentState   = D3D12_RESOURCE_STATE_COMMON;

			return;
		}

		switch (m_ResourceDesc.Dimension)
		{
		case D3D12_RESOURCE_DIMENSION_UNKNOWN:
			break;

		case D3D12_RESOURCE_DIMENSION_BUFFER:
			m_AnnouncedState = D3D12_RESOURCE_STATE_COMMON;
			m_CurrentState	 = D3D12_RESOURCE_STATE_COMMON;
			break;

		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			if (m_ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS)
			{
				m_AnnouncedState = D3D12_RESOURCE_STATE_COMMON;
				m_CurrentState   = D3D12_RESOURCE_STATE_COMMON;
			}
			else if (D3D12_RESOURCE_STATE_COMMON == m_AnnouncedState)
			{
				/*
					The following state is read-only state:
						，D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER(0x1), or
						，D3D12_RESOURCE_STATE_INDEX_BUFFER(0x2), or
						，D3D12_RESOURCE_STATE_DEPTH_READ(0x20), or
						，D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE(0x40), or
						，D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE(0x80), or
						，D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT(0x200), or 
						，D3D12_RESOURCE_STATE_COPY_SOURCE(0x800).
				*/
				if (m_CurrentState & (0x1 | 0x2 | 0x20 | 0x40 | 0x80 | 0x200 | 0x800))
				{
					m_AnnouncedState = D3D12_RESOURCE_STATE_COMMON;
					m_CurrentState	 = D3D12_RESOURCE_STATE_COMMON;
				}
			}
			break;
		}
	}

	bool CResource::NeedTransitionBarrier(const D3D12_RESOURCE_STATES& DesiredState)
	{
		if (IsOffCard() || IsCompatibleState(DesiredState))
		{
			return false;
		}

		if (IsPromotableState(DesiredState))
		{
			m_CurrentState = DesiredState;
			return false;
		}

		return true;
	}

	bool CResource::MatchSplitBarrier(const D3D12_RESOURCE_BARRIER_FLAGS& Flags)
	{
		if (m_bSplitBarrier)
		{
			if (D3D12_RESOURCE_BARRIER_FLAG_END_ONLY == Flags)
			{
				m_bSplitBarrier = false;

				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			if (D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY == Flags)
			{
				m_bSplitBarrier = true;

				return true;
			}
		}

		return false;
	}

	void CResource::TransitionBarrier(
		CCommandList* pCmdList, 
		const D3D12_RESOURCE_STATES& DesiredState,
		UINT SubResource,
		const D3D12_RESOURCE_BARRIER_FLAGS& Flags)
	{
		if (!NeedTransitionBarrier(DesiredState) || MatchSplitBarrier(Flags))
		{
			return;
		}

		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Flags = Flags;
		barrierDesc.Transition.pResource   = m_pD3D12Resource;

		if (D3D12_RESOURCE_STATE_UNORDERED_ACCESS == DesiredState)
		{
			barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barrierDesc.UAV.pResource = m_pD3D12Resource;
		}
		else
		{
			barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrierDesc.Transition.Subresource = SubResource;
			barrierDesc.Transition.StateBefore = m_CurrentState;
			barrierDesc.Transition.StateAfter = DesiredState;
		}
	
		pCmdList->ResourceBarrier(barrierDesc);

		if (!m_bSplitBarrier)
		{
			m_CurrentState = DesiredState;
		}

		m_AnnouncedState = (D3D12_RESOURCE_STATES)-1;

		DX12_ASSERT(m_CurrentState != m_AnnouncedState, "Resource barrier corruption detected!");
	}

	void CResource::Invalidate()
	{
		if (!m_pD3D12Resource)
		{
			switch (m_HeapType)
			{
			case D3D12_HEAP_TYPE_DEFAULT:
				m_pHeapAllocator->InvalidateDefaultHeapResource(m_pD3D12Resource);
				break;
			case D3D12_HEAP_TYPE_UPLOAD:
				m_pHeapAllocator->InvalidateUploadHeapResource(m_pD3D12Resource);
				break;
			case D3D12_HEAP_TYPE_READBACK:
				m_pHeapAllocator->InvalidateReadBackHeapResource(m_pD3D12Resource);
				break;
			case D3D12_HEAP_TYPE_CUSTOM:
				break;
			default:
				break;
			}
		}
	}
}