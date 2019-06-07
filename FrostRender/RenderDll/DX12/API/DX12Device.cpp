
#include"pch.h"
#include"DX12GIAdapter.h"
#include"DX12Device.h"
#include"DX12Heap.h"

namespace FrostDX12
{
#pragma region Interface Implementation
	void CDevice::AddRef()
	{
		::_InterlockedIncrement(&m_RefCount);
	}

	void CDevice::Release()
	{
		long refCount = ::_InterlockedDecrement(&m_RefCount);
		if (!refCount)
		{
			delete this;
		}
	}

	void CDevice::CreateDescriptorAllocator(
		const DESCRIPTOR_ALLOCATOR_DESC& AllocaDesc,
		IFrostDescriptorAllocator** ppDescAlloca)
	{
		if (*ppDescAlloca)
		{
			DX12_ERROR("IFrostDescriptorAllocator don't need to reallocate!");

			return;
		}

		*ppDescAlloca = DX12_NEW_RAW(CDescriptorAllocator(m_pDevice));

		static_cast<CDescriptorAllocator*>(*ppDescAlloca)->CreateDescriptorHeaps(AllocaDesc);
	}

	void CDevice::CreateHeapAllocator(
		const HEAP_ALLOCATOR_DESC& AllocaDesc,
		IFrostHeapAllocator** ppHeapAlloca)
	{
		if (*ppHeapAlloca)
		{
			DX12_ERROR("IFrostHeapAllocator don't need to reallocate!");

			return;
		}

		*ppHeapAlloca = DX12_NEW_RAW(CHeapAllocator(m_pDevice));

		static_cast<CHeapAllocator*>(*ppHeapAlloca)->CreatePlacedHeaps(AllocaDesc);
	}

	void CDevice::CreateDefaultHeapResource(
		const D3D12_RESOURCE_DESC* pDesc,
		D3D12_RESOURCE_STATES InitialState,
		const D3D12_CLEAR_VALUE* pOptimizedClearValue,
		IFrostHeapAllocator* pHeapAllocator,
		IFrostResource** ppDestResource)
	{
		if (!pHeapAllocator)
		{
			DX12_ERROR("CreateDefaultHeapResource don't allow to have no allocator!");

			return;
		}

		static_cast<CHeapAllocator*>(pHeapAllocator)->AllocateDefaultHeap(
			pDesc, InitialState, pOptimizedClearValue, ppDestResource);
	}

	void CDevice::CreateUploadHeapResource(
		const D3D12_RESOURCE_DESC* pDesc,
		const D3D12_CLEAR_VALUE* pOptimizedClearValue,
		IFrostHeapAllocator* pHeapAllocator,
		IFrostResource** ppDestResource)
	{
		if (!pHeapAllocator)
		{
			DX12_ERROR("CreateUploadHeapResource don't allow to have no allocator!");

			return;
		}

		static_cast<CHeapAllocator*>(pHeapAllocator)->AllocateUploadHeap(
			pDesc, pOptimizedClearValue, ppDestResource);
	}

	void CDevice::CreateReadBackHeap(
		const D3D12_RESOURCE_DESC* pDesc,
		const D3D12_CLEAR_VALUE* pOptimizedClearValue,
		IFrostHeapAllocator* pHeapAllocator,
		IFrostResource** ppDestResource)
	{
		if (!pHeapAllocator)
		{
			DX12_ERROR(" CreateReadBackHeap don't allow to have no allocator!");

			return;
		}

		static_cast<CHeapAllocator*>(pHeapAllocator)->AllocateReadBackHeap(
			pDesc, pOptimizedClearValue, ppDestResource);
	}

	void CDevice::CreateSampler(
		const D3D12_SAMPLER_DESC* pDesc,
		IFrostDescriptorAllocator* pDescAllocator,
		IFrostSamplerState** ppSamplerState)
	{
		if (!pDescAllocator)
		{
			DX12_ERROR("CreateSampler don't allow to have no allocator!");

			return;
		}

		static_cast<CDescriptorAllocator*>(pDescAllocator)->CacheSampler(pDesc, ppSamplerState);
	}

	void CDevice::CreateVertexBufferView(
		const D3D12_VERTEX_BUFFER_VIEW& ViewDesc,
		IFrostResource* pResource,
		IFrostView** ppIView)
	{
		if (*ppIView)
		{
			DX12_ERROR("IFrostView must be null");

			return;
		}

		*ppIView = DX12_NEW_RAW(CView());

		static_cast<CView*>(*ppIView)->CreateVBV(ViewDesc, pResource);
	}

	void CDevice::CreateIndexBufferView(
		const D3D12_INDEX_BUFFER_VIEW& ViewDesc,
		IFrostResource* pResource,
		IFrostView** ppIView)
	{
		if (*ppIView)
		{
			DX12_ERROR("IFrostView must be null");

			return;
		}

		*ppIView = DX12_NEW_RAW(CView());

		static_cast<CView*>(*ppIView)->CreateIBV(ViewDesc, pResource);
	}

	void CDevice::CreateConstantBufferView(
		const D3D12_CONSTANT_BUFFER_VIEW_DESC*  pDesc,
		IFrostDescriptorAllocator* pDescAllocator,
		IFrostResource* pResource,
		IFrostView** ppView)
	{
		if (!pDescAllocator)
		{
			DX12_ERROR("CreateConstantBufferView don't allow to have no allocator!");

			return;
		}

		static_cast<CDescriptorAllocator*>(pDescAllocator)->CacheConstantBufferView(pDesc, pResource, ppView);
	}

	void CDevice::CreateShaderResourceView(
		const D3D12_SHADER_RESOURCE_VIEW_DESC*  pDesc,
		IFrostDescriptorAllocator* pDescAllocator,
		IFrostResource* pResource,
		IFrostView** ppView)
	{
		if (!pDescAllocator)
		{
			DX12_ERROR("CreateShaderResourceView don't allow to have no allocator!");

			return;
		}

		static_cast<CDescriptorAllocator*>(pDescAllocator)->CacheShaderResourceView(pDesc, pResource, ppView);
	}

	void CDevice::CreateUnorderedAccessView(
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc,
		IFrostDescriptorAllocator* pDescAllocator,
		IFrostResource* pResource,
		IFrostView** ppView)
	{
		if (!pDescAllocator)
		{
			DX12_ERROR("CreateUnorderedAccessView don't allow to have no allocator!");

			return;
		}

		static_cast<CDescriptorAllocator*>(pDescAllocator)->CacheUnorderedAccessView(pDesc, pResource, ppView);
	}

	void CDevice::CreateDepthStencilView(
		const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc,
		IFrostDescriptorAllocator* pDescAllocator,
		IFrostResource* pResource,
		IFrostView** ppView)
	{
		if (!pDescAllocator)
		{
			DX12_ERROR("CreateDepthStencilView don't allow to have no allocator!");

			return;
		}

		static_cast<CDescriptorAllocator*>(pDescAllocator)->CacheDepthStencilView(pDesc, pResource, ppView);
	}

	void CDevice::CreateRenderTargetView(
		const D3D12_RENDER_TARGET_VIEW_DESC* pDesc,
		IFrostDescriptorAllocator* pDescAllocator,
		IFrostResource* pResource,
		IFrostView** ppView)
	{
		if (!pDescAllocator)
		{
			DX12_ERROR("CreateRenderTargetView don't allow to have no allocator!");

			return;
		}

		static_cast<CDescriptorAllocator*>(pDescAllocator)->CacheRenderTargetView(pDesc, pResource, ppView);
	}

	void CDevice::CreateRootSignature(
		const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignatureDesc,
		bool bGfx,
		DX12_PTR(IFrostRootSignature)& pIRootSignature)
	{
		m_RootSignatureCache.GetOrCreateRootSignature(pRootSignatureDesc, bGfx, GetNodeMask(), pIRootSignature);
	}

	void CDevice::CreateGraphicsPipelineState(
		const D3D12_GRAPHICS_PIPELINE_STATE_DESC& GfxPSODesc,
		DX12_PTR(IFrostPSO)& pIPSO)
	{
		m_PSOCache.GetOrCreatePSO(GfxPSODesc, pIPSO);
	}

	void CDevice::CreateComputePipelineState(
		const D3D12_COMPUTE_PIPELINE_STATE_DESC& ComputePSODesc,
		DX12_PTR(IFrostPSO)& pIPSO)
	{
		m_PSOCache.GetOrCreatePSO(ComputePSODesc, pIPSO);
	}

	void CDevice::CreateCommandList(
		int queueIndex,
		DX12_PTR(IFrostCommandList)& pCmdList)
	{
		m_CmdScheduler.GetCommandList(queueIndex, pCmdList);
	}

	void CDevice::ExecuteCommandList(
		DX12_PTR(IFrostCommandList)& pCmdList,
		bool bWait)
	{
		m_CmdScheduler.ExcuteCommandList(pCmdList,bWait);
	}

	void CDevice::HoldBack(
		int queueIndex,
		IFrostCommandList* pICmdList)
	{
		m_CmdScheduler.WaitForFinishOnGPU(queueIndex, pICmdList);
	}
#pragma endregion

	/*---------------CDevice implemtation---------------*/
	CDevice* CDevice::Create(IFrostAdapter* pAdapter)
	{
		D3D_FEATURE_LEVEL featureLevel;

		ID3D12Device* pD3D12Device = nullptr;

		IUnknown* pDXGIAdapter = pAdapter ? static_cast<CDXGIAdapter*>(pAdapter)->GetDXGIAdapter() : nullptr;

		bool createResult =
			(SUCCEEDED(D3D12CreateDevice(pDXGIAdapter, featureLevel = D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&pD3D12Device)))) ||
			(SUCCEEDED(D3D12CreateDevice(pDXGIAdapter, featureLevel = D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&pD3D12Device)))) ||
			(SUCCEEDED(D3D12CreateDevice(pDXGIAdapter, featureLevel = D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&pD3D12Device)))) ||
			(SUCCEEDED(D3D12CreateDevice(pDXGIAdapter, featureLevel = D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pD3D12Device))));

		if (!createResult)
		{
			DX12_ASSERT(0, "Failed to create D3D12 Device");

			return nullptr;

		}
		
		UINT nodeMask = static_cast<CDXGIAdapter*>(pAdapter)->GetNodeMask();
		UINT nodeCount = 1U;

		CDevice* pDevice = DX12_NEW_RAW(CDevice(pD3D12Device, featureLevel, nodeCount, nodeMask));

		pD3D12Device->Release();

		return pDevice;
	}

	CDevice::CDevice(ID3D12Device* pD3D12Device, D3D_FEATURE_LEVEL FeatureLevel, UINT NodeCount, UINT NodeMask) :
		m_pDevice(pD3D12Device),
		m_FeatureLevel(FeatureLevel),
		m_NodeCount(NodeCount),
		m_NodeMask(NodeMask),
		m_CmdScheduler(pD3D12Device, NodeMask),
		m_PSOCache(pD3D12Device),
		m_RootSignatureCache(pD3D12Device)
	{

	}

	CDevice::~CDevice()
	{
		
	}

	/*---------------CDevice functions---------------*/
	inline HRESULT STDMETHODCALLTYPE CDevice::CheckFeatureSupport(
		D3D12_FEATURE Feature, 
		void* pFeatureSupportData, 
		UINT FeatureSupportDataSize)
	{
		return m_pDevice->CheckFeatureSupport(Feature, pFeatureSupportData, Feature);
	}
}
