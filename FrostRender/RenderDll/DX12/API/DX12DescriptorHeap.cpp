#include"pch.h"
#include"DX12DescriptorHeap.h"
#include"DX12View.h"
#include"DX12SamplerState.h"
#include"DX12Resource.h"

namespace FrostDX12
{
#pragma region CDescriptorHeap Implementation
	CDescriptorHeap::CDescriptorHeap() 
	{

	}

	CDescriptorHeap::~CDescriptorHeap()
	{

	}

	/*--------------CDescriptorHeap implemtation--------------*/
	bool CDescriptorHeap::Init(ID3D12Device* pDevice, const D3D12_DESCRIPTOR_HEAP_DESC& desc)
	{
		if (!m_pDescriptorHeap)
		{
			if (FAILED(
				pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pDescriptorHeap))))
			{
				DX12_ERROR("Could not create the descriptor heap!");
			}
		
			m_DescriptorHeapDesc = m_pDescriptorHeap->GetDesc();
			m_HeapStartCPU = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			m_HeapStartGPU = m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
			m_HandleIncrementSize = pDevice->GetDescriptorHandleIncrementSize(m_DescriptorHeapDesc.Type);
		}

		Reset();
		return true;
	}
#pragma endregion

#pragma region CDescriptorAllocator Implementation

	/*--------------Interface implemtation--------------*/
	void CDescriptorAllocator::AddRef()
	{
		::_InterlockedIncrement(&m_RefCount);
	}

	void  CDescriptorAllocator::Release()
	{
		long refCount = ::_InterlockedDecrement(&m_RefCount);
		if (!refCount)
		{
			delete this;
		}
	}

	/*--------------CDescriptorAllocator implemtation--------------*/
	FrostMutexFast CDescriptorAllocator::m_SamplerThreadSafeScope;
	FrostMutexFast CDescriptorAllocator::m_CbvSrvUavThreadSafeScope;
	FrostMutexFast CDescriptorAllocator::m_DsvThreadSafeScope;
	FrostMutexFast CDescriptorAllocator::m_RtvThreadSafeScope;

	CDescriptorAllocator::CDescriptorAllocator(ID3D12Device* pDevice):
		m_SamplerHeap(),
		m_CbvSrvUavHeap(),
		m_DsvHeap(),
		m_RtvHeap(),
		m_pDevice(pDevice),
		m_RefCount(0)
	{

	}

	CDescriptorAllocator::~CDescriptorAllocator()
	{

	}

	void CDescriptorAllocator::CreateDescriptorHeaps(const DESCRIPTOR_ALLOCATOR_DESC& Descs)
	{
		m_SamplerHeap.Init(m_pDevice, Descs.SamplerDesc);
		m_CbvSrvUavHeap.Init(m_pDevice, Descs.CbvSrvUavDesc);
		m_DsvHeap.Init(m_pDevice, Descs.DepthStencilDesc);
		m_RtvHeap.Init(m_pDevice, Descs.RenderTargetDesc);
	}

	void CDescriptorAllocator::CacheSampler(
		const D3D12_SAMPLER_DESC* pDesc,
		IFrostSamplerState** samplerState)
	{
		if (*samplerState)
		{
			DX12_ERROR("The pISampler don't need to reallocate!");

			return;
		}

		*samplerState = DX12_NEW_RAW(CSamplerState(this));

		FrostAutoLock<FrostMutexFast> samplerThreadSafeScope(m_SamplerThreadSafeScope);
		{
			Hash hash = GetSamplerOrViewHash(pDesc);

			auto iterator_sampler = m_SamplerDescriptorLookupTable.find(hash);
			if (iterator_sampler == m_SamplerDescriptorLookupTable.end())
			{

				if (!m_SamplerDescriptorFreeTable.size() && (m_SamplerHeap.GetCursor() >= m_SamplerHeap.GetCapacity()))
				{
					DX12_ASSERT(false, "Sampler heap is too small!");

					return;
				}

				DESCRIPTOR_HANDLE dstHandle;
				if (m_SamplerDescriptorFreeTable.size())
				{
					dstHandle = m_SamplerDescriptorFreeTable.front();
					m_SamplerDescriptorFreeTable.pop_front();
				}
				else
				{
					dstHandle.CPUHandle = m_SamplerHeap.GetHandleOffsetCPU(0);
					dstHandle.GPUHandle = m_SamplerHeap.GetHandleOffsetGPU(0);
					m_SamplerHeap.IncrementCursor();
				}

				m_pDevice->CreateSampler(pDesc, dstHandle.CPUHandle);

				auto insertResult = m_SamplerDescriptorLookupTable.emplace(hash, dstHandle);
				DX12_ASSERT(insertResult.second);
				iterator_sampler = insertResult.first;
			}

			static_cast<CSamplerState*>(*samplerState)->SetSamplerDesc(*pDesc);
			static_cast<CSamplerState*>(*samplerState)->SetDescriptorHandle(iterator_sampler->second);

			return;
		}
	}

	void CDescriptorAllocator::CacheConstantBufferView(
		const D3D12_CONSTANT_BUFFER_VIEW_DESC* pDesc,
		IFrostResource* pIResource,
		IFrostView** ppIView)
	{
		if (*ppIView)
		{
			DX12_ERROR("The pICBV don't need to reallocate!");

			return;
		}

		*ppIView = DX12_NEW_RAW(CView(this));

		auto pResource = static_cast<CResource*>(pIResource);
		auto pView = static_cast<CView*>(*ppIView);

		FrostAutoLock<FrostMutexFast> ConstantBufferThreadSafeScope(m_CbvSrvUavThreadSafeScope);
		{
			Hash hash = GetSamplerOrViewHash(pDesc);

			auto iterator_cbv = m_CbvSrvUavDescriptorLookupTable.find(hash);
			if (iterator_cbv == m_CbvSrvUavDescriptorLookupTable.end())
			{

				if (!m_CbvSrvUavDescriptorFreeTable.size() &&
					(m_CbvSrvUavHeap.GetCursor() >= m_CbvSrvUavHeap.GetCapacity()))
				{
					DX12_ASSERT(false, "ConstantBuffer heap is too small!");

					return;
				}

				DESCRIPTOR_HANDLE dstHandle;
				if (m_CbvSrvUavDescriptorFreeTable.size())
				{
					dstHandle = m_CbvSrvUavDescriptorFreeTable.front();
					m_CbvSrvUavDescriptorFreeTable.pop_front();
				}
				else
				{
					dstHandle.CPUHandle = m_CbvSrvUavHeap.GetHandleOffsetCPU(0);
					dstHandle.GPUHandle = m_CbvSrvUavHeap.GetHandleOffsetGPU(0);
					m_CbvSrvUavHeap.IncrementCursor();
				}

				m_pDevice->CreateConstantBufferView(pDesc, dstHandle.CPUHandle);

				auto insertResult = m_CbvSrvUavDescriptorLookupTable.emplace(hash, dstHandle);
				DX12_ASSERT(insertResult.second);
				iterator_cbv = insertResult.first;
			}

			pView->CreateCBV(pDesc, pResource);
			pView->SetDescriptorHandle(iterator_cbv->second);

			return;
		}
	}

	void CDescriptorAllocator::CacheShaderResourceView(
		const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc,
		IFrostResource* pIResource,
		IFrostView** ppIView)
	{
		if (*ppIView)
		{
			DX12_ERROR("The pISRV don't need to reallocate!");

			return;
		}

		*ppIView = DX12_NEW_RAW(CView(this));

		auto pResource = pIResource ? static_cast<CResource*>(pIResource)->GetD3D12Resource() : nullptr;
		auto pView = static_cast<CView*>(*ppIView);

		FrostAutoLock<FrostMutexFast> ShaderResourceThreadSafeScope(m_CbvSrvUavThreadSafeScope);
		{
			Hash hash = GetSamplerOrViewHash(pDesc, pResource);

			auto iterator_srv = m_CbvSrvUavDescriptorLookupTable.find(hash);
			if (!pResource || iterator_srv == m_CbvSrvUavDescriptorLookupTable.end())
			{

				if (!m_CbvSrvUavDescriptorFreeTable.size() &&
					(m_CbvSrvUavHeap.GetCursor() >= m_CbvSrvUavHeap.GetCapacity()))
				{
					DX12_ASSERT(false, "ShaderResource heap is too small!");

					return;
				}

				DESCRIPTOR_HANDLE dstHandle;
				if (m_CbvSrvUavDescriptorFreeTable.size())
				{
					dstHandle = m_CbvSrvUavDescriptorFreeTable.front();
					m_CbvSrvUavDescriptorFreeTable.pop_front();
				}
				else
				{
					dstHandle.CPUHandle = m_CbvSrvUavHeap.GetHandleOffsetCPU(0);
					dstHandle.GPUHandle = m_CbvSrvUavHeap.GetHandleOffsetGPU(0);
					m_CbvSrvUavHeap.IncrementCursor();
				}

				m_pDevice->CreateShaderResourceView(
					pResource,
					pDesc,
					dstHandle.CPUHandle);

				auto insertResult = m_CbvSrvUavDescriptorLookupTable.emplace(hash, dstHandle);

				if (pResource)
				{
					DX12_ASSERT(insertResult.second);
				}

				iterator_srv = insertResult.first;
			}

			pView->CreateSRV(pDesc, pIResource);
			pView->SetDescriptorHandle(iterator_srv->second);

			return;
		}
	}

	void CDescriptorAllocator::CacheUnorderedAccessView(
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc,
		IFrostResource* pIResource,
		IFrostView** ppIView)
	{
		if (*ppIView)
		{
			DX12_ERROR("The pIUAV don't need to reallocate!");

			return;
		}
		*ppIView = DX12_NEW_RAW(CView(this));

		auto pResource = pIResource ? static_cast<CResource*>(pIResource)->GetD3D12Resource() : nullptr;
		auto pView = static_cast<CView*>(*ppIView);

		FrostAutoLock<FrostMutexFast> UnorderedAccessThreadSafeScope(m_CbvSrvUavThreadSafeScope);
		{
			Hash hash = GetSamplerOrViewHash(pDesc, pResource);

			auto iterator_uav = m_CbvSrvUavDescriptorLookupTable.find(hash);
			if (iterator_uav == m_CbvSrvUavDescriptorLookupTable.end())
			{

				if (!m_CbvSrvUavDescriptorFreeTable.size() &&
					(m_CbvSrvUavHeap.GetCursor() >= m_CbvSrvUavHeap.GetCapacity()))
				{
					DX12_ASSERT(false, "UnorderedAccess heap is too small!");

					return;
				}

				DESCRIPTOR_HANDLE dstHandle;
				if (m_CbvSrvUavDescriptorFreeTable.size())
				{
					dstHandle = m_CbvSrvUavDescriptorFreeTable.front();
					m_CbvSrvUavDescriptorFreeTable.pop_front();
				}
				else
				{
					dstHandle.CPUHandle = m_CbvSrvUavHeap.GetHandleOffsetCPU(0);
					dstHandle.GPUHandle = m_CbvSrvUavHeap.GetHandleOffsetGPU(0);
					m_CbvSrvUavHeap.IncrementCursor();
				}

				m_pDevice->CreateUnorderedAccessView(pResource, nullptr, pDesc, dstHandle.CPUHandle);

				auto insertResult = m_CbvSrvUavDescriptorLookupTable.emplace(hash, dstHandle);
				DX12_ASSERT(insertResult.second);
				iterator_uav = insertResult.first;
			}

			pView->CreateUAV(pDesc, pIResource);
			pView->SetDescriptorHandle(iterator_uav->second);

		}
	}

	void CDescriptorAllocator::CacheDepthStencilView(
		const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc,
		IFrostResource* pIResource,
		IFrostView** ppIView)
	{
		if (*ppIView)
		{
			DX12_ERROR("The pIDSV don't need to reallocate!");

			return;
		}

		*ppIView = DX12_NEW_RAW(CView(this));

		auto pResource = pIResource ? static_cast<CResource*>(pIResource)->GetD3D12Resource() : nullptr;
		auto pView = static_cast<CView*>(*ppIView);

		FrostAutoLock<FrostMutexFast> DepthStencilThreadSafeScope(m_DsvThreadSafeScope);
		{
			Hash hash = GetSamplerOrViewHash(pDesc, pResource);
			auto iterator_dsv = m_DsvDescriptorLookupTable.find(hash);

			if (iterator_dsv == m_DsvDescriptorLookupTable.end())
			{
				if (!m_DsvDescriptorFreeTable.size() &&
					(m_DsvHeap.GetCursor() >= m_DsvHeap.GetCapacity()))
				{
					DX12_ASSERT(false, "DepthStencil heap is too small!");

					return;
				}

				DESCRIPTOR_HANDLE dstHandle;
				if (m_DsvDescriptorFreeTable.size())
				{
					dstHandle = m_DsvDescriptorFreeTable.front();
					m_DsvDescriptorFreeTable.pop_front();
				}
				else
				{
					dstHandle.CPUHandle = m_DsvHeap.GetHandleOffsetCPU(0);
					dstHandle.GPUHandle = m_DsvHeap.GetHandleOffsetGPU(0);
					m_DsvHeap.IncrementCursor();
				}

				m_pDevice->CreateDepthStencilView(pResource, pDesc, dstHandle.CPUHandle);

				auto insertResult = m_DsvDescriptorLookupTable.emplace(hash, dstHandle);
				DX12_ASSERT(insertResult.second);
				iterator_dsv = insertResult.first;
			}

			pView->CreateDSV(pDesc, pIResource);
			pView->SetDescriptorHandle(iterator_dsv->second);

			return;
		}
	}

	void CDescriptorAllocator::CacheRenderTargetView(
		const D3D12_RENDER_TARGET_VIEW_DESC* pDesc,
		IFrostResource* pIResource,
		IFrostView** ppIView)
	{
		if (*ppIView)
		{
			DX12_ERROR("The pIRTV don't need to reallocate!");

			return;
		}

		*ppIView = DX12_NEW_RAW(CView(this));

		auto pResource = pIResource ? static_cast<CResource*>(pIResource)->GetD3D12Resource() : nullptr;
		auto pView = static_cast<CView*>(*ppIView);

		FrostAutoLock<FrostMutexFast> RenderTargetThreadSafeScope(m_RtvThreadSafeScope);
		{
			Hash hash = GetSamplerOrViewHash(pDesc, pResource);

			auto iterator_rtv = m_RtvDescriptorLookupTable.find(hash);
			if (iterator_rtv == m_RtvDescriptorLookupTable.end())
			{

				if (!m_RtvDescriptorFreeTable.size() &&
					(m_RtvHeap.GetCursor() >= m_RtvHeap.GetCapacity()))
				{
					DX12_ASSERT(false, "RenderTarget heap is too small!");

					return;
				}

				DESCRIPTOR_HANDLE dstHandle;
				if (m_RtvDescriptorFreeTable.size())
				{
					dstHandle = m_RtvDescriptorFreeTable.front();
					m_RtvDescriptorFreeTable.pop_front();
				}
				else
				{
					dstHandle.CPUHandle = m_RtvHeap.GetHandleOffsetCPU(0);
					dstHandle.GPUHandle = m_RtvHeap.GetHandleOffsetGPU(0);
					m_RtvHeap.IncrementCursor();
				}

				m_pDevice->CreateRenderTargetView(pResource, pDesc, dstHandle.CPUHandle);

				auto insertResult = m_RtvDescriptorLookupTable.emplace(hash, dstHandle);
				DX12_ASSERT(insertResult.second);
				iterator_rtv = insertResult.first;
			}

			pView->CreateRTV(pDesc, pIResource);
			pView->SetDescriptorHandle(iterator_rtv->second);

			return;
		}
	}

	void CDescriptorAllocator::CacheNullShaderResourceView(IFrostView** ppIView)
	{
		if (*ppIView)
		{
			DX12_ERROR("The pIRTV don't need to reallocate!");

			return;
		}

		*ppIView = DX12_NEW_RAW(CView(this));

		DESCRIPTOR_HANDLE dstHandle;

		FrostAutoLock<FrostMutexFast> ShaderResourceThreadSafeScope(m_CbvSrvUavThreadSafeScope);
		{
			dstHandle.CPUHandle = m_CbvSrvUavHeap.GetHandleOffsetCPU(0);
			dstHandle.GPUHandle = m_CbvSrvUavHeap.GetHandleOffsetGPU(0);
			m_CbvSrvUavHeap.IncrementCursor();
		}
		
		static_cast<CView*>(*ppIView)->SetDescriptorHandle(dstHandle);
	}


	void CDescriptorAllocator::InvalidateSampler(const D3D12_SAMPLER_DESC* pDesc)
	{
		FrostAutoLock<FrostMutexFast> samplerThreadSafeScope(m_SamplerThreadSafeScope);

		auto iterator_sampler = m_SamplerDescriptorLookupTable.find(GetSamplerOrViewHash(pDesc));
		if (iterator_sampler != m_SamplerDescriptorLookupTable.end())
		{
			m_SamplerDescriptorFreeTable.push_back(iterator_sampler->second);
			m_SamplerDescriptorLookupTable.erase(iterator_sampler);
		}
	}

	void CDescriptorAllocator::InvalidateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC* pDesc)
	{
		FrostAutoLock<FrostMutexFast> constantBufferThreadSafeScope(m_CbvSrvUavThreadSafeScope);

		auto iterator_cbv = m_CbvSrvUavDescriptorLookupTable.find(GetSamplerOrViewHash(pDesc));
		if (iterator_cbv != m_CbvSrvUavDescriptorLookupTable.end())
		{
			m_CbvSrvUavDescriptorFreeTable.push_back(iterator_cbv->second);
			m_CbvSrvUavDescriptorLookupTable.erase(iterator_cbv);
		}
	}

	void CDescriptorAllocator::InvalidateShaderResourceView(
		const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc, 
		ID3D12Resource* pResouce)
	{
		FrostAutoLock<FrostMutexFast> ShaderResourceThreadSafeScope(m_CbvSrvUavThreadSafeScope);

		auto iterator_srv = m_CbvSrvUavDescriptorLookupTable.find(GetSamplerOrViewHash(pDesc,pResouce));
		if (iterator_srv != m_CbvSrvUavDescriptorLookupTable.end())
		{
			m_CbvSrvUavDescriptorFreeTable.push_back(iterator_srv->second);
			m_CbvSrvUavDescriptorLookupTable.erase(iterator_srv);
		}
	}

	void CDescriptorAllocator::InvalidateUnorderedAccessView(
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc, 
		ID3D12Resource* pResouce)
	{
		FrostAutoLock<FrostMutexFast> unorderedAccessThreadSafeScope(m_CbvSrvUavThreadSafeScope);

		auto iterator_uav = m_CbvSrvUavDescriptorLookupTable.find(GetSamplerOrViewHash(pDesc, pResouce));
		if (iterator_uav != m_CbvSrvUavDescriptorLookupTable.end())
		{
			m_CbvSrvUavDescriptorFreeTable.push_back(iterator_uav->second);
			m_CbvSrvUavDescriptorLookupTable.erase(iterator_uav);
		}
	}

	void CDescriptorAllocator::InvalidateDepthStencilView(
		const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc, 
		ID3D12Resource* pResouce)
	{
		FrostAutoLock<FrostMutexFast> depthStencilThreadSafeScope(m_DsvThreadSafeScope);

		auto iterator_dsv = m_DsvDescriptorLookupTable.find(GetSamplerOrViewHash(pDesc, pResouce));
		if (iterator_dsv != m_DsvDescriptorLookupTable.end())
		{
			m_DsvDescriptorFreeTable.push_back(iterator_dsv->second);
			m_DsvDescriptorLookupTable.erase(iterator_dsv);
		}
	}

	void CDescriptorAllocator::InvalidateRenderTargetView(
		const D3D12_RENDER_TARGET_VIEW_DESC* pDesc, 
		ID3D12Resource* pResouce)
	{
		FrostAutoLock<FrostMutexFast> renderTargetThreadSafeScope(m_RtvThreadSafeScope);

		auto iterator_rtv = m_RtvDescriptorLookupTable.find(GetSamplerOrViewHash(pDesc, pResouce));
		if (iterator_rtv != m_RtvDescriptorLookupTable.end())
		{
			m_RtvDescriptorFreeTable.push_back(iterator_rtv->second);
			m_RtvDescriptorLookupTable.erase(iterator_rtv);
		}
	}
#pragma endregion
}