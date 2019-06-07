#pragma once

#include"DX12\Interface\FrostRender.h"
#include"DX12Base.h"

namespace FrostDX12
{
	class CResource;

	class CHeap : public IRefCounted
	{
	public:
		virtual void AddRef()  override;
		virtual void Release() override;

	public:
		CHeap(ID3D12Device* pDevice);
		~CHeap() noexcept;

		void Init(const D3D12_HEAP_DESC& HeapDesc);
		
		ID3D12Heap* GetD3D12Heap()const		  { return m_pHeap; }
		UINT64		GetHeapSize()const		  { return m_HeapDesc.SizeInBytes; }
		UINT		GetHeapType()const		  { return m_HeapDesc.Properties.Type; }
		UINT64		GetHeapAlignment()const   { return m_HeapDesc.Alignment; }

		UINT64		GetCursor()const		  { return m_Cursor; }
		void		SetCursor(UINT64 Cursor)  { m_Cursor = Cursor; }

		bool		IsOutOfBound(UINT64 Size) { return m_Cursor + Size > GetHeapSize(); }
	private:
		DX12_PTR(ID3D12Heap) m_pHeap;

		ID3D12Device*		 m_pDevice;

		D3D12_HEAP_DESC		 m_HeapDesc;
		UINT64				 m_Cursor;

		volatile long		 m_RefCount;
	};

	class CHeapBlock 
	{
	public:
		CHeapBlock() : m_BlockStart(0), m_Capacity(0) {}

		CHeapBlock(CHeap* pHeap, UINT64 BlockStart, UINT64 Capacity) :
			m_pHeap(pHeap),
			m_BlockStart(BlockStart),
			m_Capacity(Capacity)
		{
			DX12_ASSERT(BlockStart + Capacity <= pHeap->GetHeapSize());
		}
		
		~CHeapBlock() noexcept {}

		CHeap* GetHeap()const		 { return m_pHeap; }
		UINT64 GetStartOffset()const { return m_BlockStart; }
		UINT64 GetCapacity()const	 { return m_Capacity; }

	private:
		CHeapBlock(const CHeapBlock&) = delete;
		CHeapBlock& operator=(const CHeapBlock&) = delete;

		DX12_PTR(CHeap) m_pHeap;
		UINT64 m_BlockStart;
		UINT64 m_Capacity;	
	};

	class CHeapAllocator : public IFrostHeapAllocator
	{
	public:
		virtual void AddRef()  override;
		virtual void Release() override;

	public:
		CHeapAllocator(ID3D12Device* pDevice);
		~CHeapAllocator() noexcept;

		void CreatePlacedHeaps(const HEAP_ALLOCATOR_DESC& HeapDesc);

		void AllocateDefaultHeap(
			const D3D12_RESOURCE_DESC* pDesc,
			D3D12_RESOURCE_STATES InitialState,
			const D3D12_CLEAR_VALUE* pOptimizedClearValue,
			IFrostResource** ppDestResource);

		void AllocateUploadHeap(
			const D3D12_RESOURCE_DESC* pDesc,
			const D3D12_CLEAR_VALUE* pOptimizedClearValue,
			IFrostResource** ppDestResource);

		void AllocateReadBackHeap(
			const D3D12_RESOURCE_DESC* pDesc,
			const D3D12_CLEAR_VALUE* pOptimizedClearValue,
			IFrostResource** ppDestResource);

		void InvalidateDefaultHeapResource (ID3D12Resource* pResource);

		void InvalidateUploadHeapResource  (ID3D12Resource* pResource);

		void InvalidateReadBackHeapResource(ID3D12Resource* pResource);

	private:
		ID3D12Device* m_pDevice;

		CHeap	m_DefaultHeap;
		CHeap	m_UploadHeap;
		CHeap	m_ReadBackHeap;

		std::list<CHeapBlock*> m_DefaultHeapFreeTable;
		std::list<CHeapBlock*> m_UploadHeapFreeTable;
		std::list<CHeapBlock*> m_ReadBackHeapFreeTable;

		static FrostMutexFast m_DefaultHeapThreadSafeScope;
		static FrostMutexFast m_UploadHeapThreadSafeScope;
		static FrostMutexFast m_ReadBackHeapThreadSafeScope;

		typedef std::unordered_map<ID3D12Resource*, CHeapBlock*> ResouceHeapBlockMap;
		ResouceHeapBlockMap m_DefaultHeapLookupTable;
		ResouceHeapBlockMap m_UploadHeapLookupTable;
		ResouceHeapBlockMap m_ReadBackHeapLookupTable;

		volatile long m_RefCount;
	};
}