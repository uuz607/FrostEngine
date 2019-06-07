#pragma once

#include"DX12\Interface\FrostRenderDXGI.h"
#include"DX12Base.h"

namespace FrostDX12
{
	class CView;
	class CSamplerState;
	class CResource;

	struct SCursorHost
	{
		SCursorHost() : m_Cursor(0) {}
		SCursorHost(SCursorHost&& other) : m_Cursor(std::move(other.m_Cursor)) {}
		SCursorHost& operator=(SCursorHost&& other) { m_Cursor = std::move(other.m_Cursor); return *this; }

		void SetCursor(UINT value)			{ m_Cursor = value; }
		UINT GetCursor() const				{ return m_Cursor; }
		void IncrementCursor(UINT step = 1) { m_Cursor += step; }
		void Reset()						{ m_Cursor = 0; }

		UINT m_Cursor;
	};

	class CDescriptorHeap : public SCursorHost
	{
	public:
		CDescriptorHeap();
		~CDescriptorHeap();

		bool Init(ID3D12Device* pDevice, const D3D12_DESCRIPTOR_HEAP_DESC& desc);

		ID3D12DescriptorHeap* GetD3D12DescriptorHeap() const
		{
			return m_pDescriptorHeap;
		}

		UINT GetHandleIncrementSize() const
		{
			return m_HandleIncrementSize;
		}

		UINT GetCapacity() const
		{
			return m_DescriptorHeapDesc.NumDescriptors;
		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE GetHandleOffsetCPU(INT offset) const
		{
			return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_HeapStartCPU, m_Cursor + offset, m_HandleIncrementSize);
		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE GetHandleOffsetCPU_R(INT offset) const
		{
			return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_HeapStartCPU, offset, m_HandleIncrementSize);
		}

		CD3DX12_GPU_DESCRIPTOR_HANDLE GetHandleOffsetGPU(INT offset) const
		{
			return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_HeapStartGPU, m_Cursor + offset, m_HandleIncrementSize);
		}

		CD3DX12_GPU_DESCRIPTOR_HANDLE GetHandleOffsetGPU_R(INT offset) const
		{
			return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_HeapStartGPU, offset, m_HandleIncrementSize);
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPUFromCPU(D3D12_CPU_DESCRIPTOR_HANDLE handle) const
		{
			D3D12_GPU_DESCRIPTOR_HANDLE rebase;
			rebase.ptr = m_HeapStartGPU.ptr + UINT64(handle.ptr - m_HeapStartCPU.ptr);
			return rebase;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPUFromGPU(D3D12_GPU_DESCRIPTOR_HANDLE handle) const
		{
			D3D12_CPU_DESCRIPTOR_HANDLE rebase;
			rebase.ptr = m_HeapStartCPU.ptr + SIZE_T(handle.ptr - m_HeapStartGPU.ptr);
			return rebase;
		}

	private:
		DX12_PTR(ID3D12DescriptorHeap) m_pDescriptorHeap;
		D3D12_DESCRIPTOR_HEAP_DESC	   m_DescriptorHeapDesc;
		D3D12_CPU_DESCRIPTOR_HANDLE    m_HeapStartCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE    m_HeapStartGPU;
		UINT                           m_HandleIncrementSize;
	};

	class CDescriptorAllocator : public IFrostDescriptorAllocator
	{
	public:
		virtual void AddRef()  override;
		virtual void Release() override;

		void OffsetInSamplerDescriptors(INT offsetInDescriptors)override
		{
			GetSamplerHeap()->IncrementCursor(offsetInDescriptors);
		}

		void OffsetInCbvSrvUavDescriptors(INT offsetInDescriptors) override
		{
			GetCbvSrvUavHeap()->IncrementCursor(offsetInDescriptors);
		}

		void OffsetInDsvDescriptors(INT offsetInDescriptors) override
		{
			GetDsvHeap()->IncrementCursor(offsetInDescriptors);
		}

		void OffsetInRtvDescriptors(INT offsetInDescriptors) override
		{
			GetRtvHeap()->IncrementCursor(offsetInDescriptors);
		}

	public:
		CDescriptorAllocator(ID3D12Device* pDevice);
		~CDescriptorAllocator();
		
		void CreateDescriptorHeaps(const DESCRIPTOR_ALLOCATOR_DESC& Descs);

		CDescriptorHeap* GetSamplerHeap()  { return &m_SamplerHeap; }
		CDescriptorHeap* GetCbvSrvUavHeap(){ return &m_CbvSrvUavHeap; }
		CDescriptorHeap* GetDsvHeap()      { return &m_DsvHeap; }
		CDescriptorHeap* GetRtvHeap()      { return &m_RtvHeap; }

		template<typename T>
		static Hash GetSamplerOrViewHash(const T* pDesc, ID3D12Resource* pResource)
		{
			if (!pDesc)
			{
				return (UINT32)((UINT64)pResource);
			}

			return ComputeSmallHash<sizeof(T)>(pDesc, (UINT32)((UINT64)pResource));
		}

		template<typename T>
		static Hash GetSamplerOrViewHash(const T* pDesc)
		{
			return ComputeSmallHash<sizeof(T)>(pDesc);
		}

		//The way to create sampler or views is thread-safe;
		void CacheSampler(const D3D12_SAMPLER_DESC* pDesc, IFrostSamplerState** samplerState);
		void CacheConstantBufferView (const D3D12_CONSTANT_BUFFER_VIEW_DESC*  pDesc, IFrostResource* pIResource, IFrostView** ppIView);
		void CacheShaderResourceView (const D3D12_SHADER_RESOURCE_VIEW_DESC*  pDesc, IFrostResource* pIResource, IFrostView** ppIView);
		void CacheUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc, IFrostResource* pIResource, IFrostView** ppIView);
		void CacheDepthStencilView	 (const D3D12_DEPTH_STENCIL_VIEW_DESC*	  pDesc, IFrostResource* pIResource, IFrostView** ppIView);
		void CacheRenderTargetView	 (const D3D12_RENDER_TARGET_VIEW_DESC*	  pDesc, IFrostResource* pIResource, IFrostView** ppIView);
		void CacheNullShaderResourceView(IFrostView** ppIView);

		void InvalidateSampler			  (const D3D12_SAMPLER_DESC* pDesc);
		void InvalidateConstantBufferView (const D3D12_CONSTANT_BUFFER_VIEW_DESC*  pDesc);
		void InvalidateShaderResourceView (const D3D12_SHADER_RESOURCE_VIEW_DESC*  pDesc, ID3D12Resource* pResource);
		void InvalidateUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc, ID3D12Resource* pResource);
		void InvalidateDepthStencilView   (const D3D12_DEPTH_STENCIL_VIEW_DESC*    pDesc, ID3D12Resource* pResource);
		void InvalidateRenderTargetView   (const D3D12_RENDER_TARGET_VIEW_DESC*    pDesc, ID3D12Resource* pResource);

	private:
		static FrostMutexFast m_SamplerThreadSafeScope;
		static FrostMutexFast m_CbvSrvUavThreadSafeScope;
		static FrostMutexFast m_DsvThreadSafeScope;
		static FrostMutexFast m_RtvThreadSafeScope;
		
		CDescriptorHeap m_SamplerHeap;
		CDescriptorHeap m_CbvSrvUavHeap;
		CDescriptorHeap m_DsvHeap;
		CDescriptorHeap m_RtvHeap;
		
		typedef std::unordered_map<Hash, DESCRIPTOR_HANDLE> DescriptorMap;

		DescriptorMap m_SamplerDescriptorLookupTable;
		DescriptorMap m_CbvSrvUavDescriptorLookupTable;
		DescriptorMap m_DsvDescriptorLookupTable;
		DescriptorMap m_RtvDescriptorLookupTable;
	

		std::list<DESCRIPTOR_HANDLE> m_SamplerDescriptorFreeTable;
		std::list<DESCRIPTOR_HANDLE> m_CbvSrvUavDescriptorFreeTable;
		std::list<DESCRIPTOR_HANDLE> m_DsvDescriptorFreeTable;
		std::list<DESCRIPTOR_HANDLE> m_RtvDescriptorFreeTable;

		ID3D12Device* m_pDevice;

		volatile long m_RefCount;
	};
}