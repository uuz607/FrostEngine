#include"pch.h"
#include"DX12Heap.h"
#include"DX12Resource.h"

namespace FrostDX12
{
	/*--------------Interface implemtation--------------*/
	void CHeap::AddRef()
	{
		::_InterlockedIncrement(&m_RefCount);
	}

	void CHeap::Release()
	{
		long refCount = ::_InterlockedDecrement(&m_RefCount);
		if (!refCount)
		{
			delete this;
		}
	}

	/*--------------CHeap implemtation--------------*/
	CHeap::CHeap(ID3D12Device* pDevice) :
		m_pDevice(pDevice),
		m_Cursor(0),
		m_RefCount(0)
	{

	}

	CHeap::~CHeap()
	{

	}

	void CHeap::Init(const D3D12_HEAP_DESC& HeapDesc)
	{
		if (!m_pHeap)
		{
			m_pDevice->CreateHeap(&HeapDesc, IID_PPV_ARGS(&m_pHeap));

			m_HeapDesc = m_pHeap->GetDesc();

	#ifdef _DEBUG
			switch (m_HeapDesc.Properties.Type)
			{
			case D3D12_HEAP_TYPE_DEFAULT:
				m_pHeap->SetName(L"Default Heap");
				break;
			case D3D12_HEAP_TYPE_UPLOAD:
				m_pHeap->SetName(L"Upload Heap");
				break;
			case D3D12_HEAP_TYPE_READBACK:
				m_pHeap->SetName(L"Readback Heap");
				break;
			default:
				break;
			}
	#endif 
		}
	}


#pragma region CHeapAllocator Implementation

	/*--------------Interface implemtation--------------*/
	void CHeapAllocator::AddRef()
	{
		::_InterlockedIncrement(&m_RefCount);
	}

	void CHeapAllocator::Release()
	{
		long refCount = ::_InterlockedDecrement(&m_RefCount);
		if (!refCount)
		{
			delete this;
		}
	}

	/*--------------CHeapAllocator implemtation--------------*/
	FrostMutexFast CHeapAllocator::m_DefaultHeapThreadSafeScope;
	FrostMutexFast CHeapAllocator::m_UploadHeapThreadSafeScope;
	FrostMutexFast CHeapAllocator::m_ReadBackHeapThreadSafeScope;

	CHeapAllocator::CHeapAllocator(ID3D12Device* pDevice) :
		m_pDevice(pDevice),
		m_DefaultHeap(pDevice),
		m_UploadHeap(pDevice),
		m_ReadBackHeap(pDevice),
		m_RefCount(0)
	{
	
	}
		
	CHeapAllocator::~CHeapAllocator()
	{

	}

	void CHeapAllocator::CreatePlacedHeaps(const HEAP_ALLOCATOR_DESC& HeapDesc)
	{
		m_DefaultHeap.Init(HeapDesc.DefaultHeapDesc);
		m_UploadHeap.Init(HeapDesc.UploadHeapDesc);
		m_ReadBackHeap.Init(HeapDesc.ReadBackHeapDesc);
	}


	/*--------------AllocaterDefaultHeap function--------------*/
	void CHeapAllocator::AllocateDefaultHeap(
		const D3D12_RESOURCE_DESC* pDesc,
		D3D12_RESOURCE_STATES InitialState,
		const D3D12_CLEAR_VALUE* pOptimizedClearValue,
		IFrostResource** ppDestResource)
	{
		if (*ppDestResource)
		{
			DX12_ERROR("The allocated pointer don't need to reallocate!");

			return;
		}

		*ppDestResource = DX12_NEW_RAW(CResource(m_pDevice, this));

		ID3D12Resource* pResource = nullptr;

		auto allocaInfo = m_pDevice->GetResourceAllocationInfo(1U, 1U, pDesc);
		UINT64 size = allocaInfo.SizeInBytes;

		FrostAutoLock<FrostMutexFast> threadSafeScope(m_DefaultHeapThreadSafeScope);
		{
			if (m_DefaultHeapFreeTable.size())
			{
				for (auto it = m_DefaultHeapFreeTable.begin(); it != m_DefaultHeapFreeTable.end(); ++it)
				{
					if ((*it)->GetCapacity() >= size)
					{
						if (SUCCEEDED(m_pDevice->CreatePlacedResource(
							m_DefaultHeap.GetD3D12Heap(),
							(*it)->GetStartOffset(),
							pDesc, InitialState, pOptimizedClearValue,
							IID_PPV_ARGS(&pResource))))
						{
							m_DefaultHeapLookupTable[pResource] = *it;
							m_DefaultHeapFreeTable.erase(it);
							break;
						}
						else
						{
							DX12_ERROR("Could not create placed resource from the default heap!");
						}
					
					}
				}
			}
			else if (!m_DefaultHeap.IsOutOfBound(size))
			{
				if (SUCCEEDED(m_pDevice->CreatePlacedResource(
					m_DefaultHeap.GetD3D12Heap(),
					m_DefaultHeap.GetCursor(),
					pDesc, InitialState, pOptimizedClearValue,
					IID_PPV_ARGS(&pResource))))
				{
					m_DefaultHeap.SetCursor(m_DefaultHeap.GetCursor() + size);

					CHeapBlock* block = new CHeapBlock(&m_DefaultHeap, m_DefaultHeap.GetCursor(), size);

					m_DefaultHeapLookupTable[pResource] = block;
				}
				else
				{
					DX12_ERROR("Could not create placed resource from the default heap!");
				}	
			}
			else
			{
				if (FAILED(m_pDevice->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
					D3D12_HEAP_FLAG_NONE,
					pDesc, InitialState, pOptimizedClearValue,
					IID_PPV_ARGS(&pResource))))
				{
					DX12_ERROR("Could not create committed resource!");
				}
			}
		}

		static_cast<CResource*>(*ppDestResource)->Init(pResource, InitialState);

		pResource->Release();
	}
	
	/*--------------AllocaterUploadHeap function--------------*/
	void CHeapAllocator::AllocateUploadHeap(
		const D3D12_RESOURCE_DESC* pDesc,
		const D3D12_CLEAR_VALUE* pOptimizedClearValue,
		IFrostResource** ppDestResource)
	{
		if (*ppDestResource)
		{
			DX12_ERROR("The allocated pointer don't need to reallocate!");

			return;
		}

		*ppDestResource = DX12_NEW_RAW(CResource(m_pDevice, this));

		ID3D12Resource* pResource = nullptr;

		auto allocaInfo = m_pDevice->GetResourceAllocationInfo(1U, 1U, pDesc);
		UINT64 size = allocaInfo.SizeInBytes;

		FrostAutoLock<FrostMutexFast> threadSafeScope(m_UploadHeapThreadSafeScope);
		{
			if (m_UploadHeapFreeTable.size())
			{
				for (auto it = m_UploadHeapFreeTable.begin(); it != m_UploadHeapFreeTable.end(); ++it)
				{
					if ((*it)->GetCapacity() >= size)
					{
						if (SUCCEEDED(m_pDevice->CreatePlacedResource(
							m_UploadHeap.GetD3D12Heap(),
							(*it)->GetStartOffset(),
							pDesc,
							D3D12_RESOURCE_STATE_GENERIC_READ,
							pOptimizedClearValue,
							IID_PPV_ARGS(&pResource))))
						{
							m_UploadHeapLookupTable[pResource] = *it;
							m_UploadHeapFreeTable.erase(it);
							break;		
						}
						else
						{
							DX12_ERROR("Could not create placed resource from the upload heap!");
						}
					}
				}
			}
			else if (!m_UploadHeap.IsOutOfBound(size))
			{
				if (SUCCEEDED(m_pDevice->CreatePlacedResource(
					m_UploadHeap.GetD3D12Heap(),
					m_UploadHeap.GetCursor(),
					pDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					pOptimizedClearValue,
					IID_PPV_ARGS(&pResource))))
				{
					m_UploadHeap.SetCursor(m_UploadHeap.GetCursor() + size);

					CHeapBlock* block = new CHeapBlock(&m_UploadHeap, m_UploadHeap.GetCursor(), size);

					m_UploadHeapLookupTable[pResource] = block;
				}
				else
				{
					DX12_ERROR("Could not create placed resource from the upload heap!");
				}
			}
			else
			{
				if (FAILED(m_pDevice->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
					D3D12_HEAP_FLAG_NONE,
					pDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					pOptimizedClearValue,
					IID_PPV_ARGS(&pResource))))
				{
					DX12_ERROR("Could not create committed resource!");
				}
			}
		}

		static_cast<CResource*>(*ppDestResource)->Init(pResource, D3D12_RESOURCE_STATE_GENERIC_READ);

		pResource->Release();
	}

	/*--------------AllocaterReadBackHeap function--------------*/
	void CHeapAllocator::AllocateReadBackHeap(
		const D3D12_RESOURCE_DESC* pDesc,
		const D3D12_CLEAR_VALUE* pOptimizedClearValue,
		IFrostResource** ppDestResource)
	{
		if (*ppDestResource)
		{
			DX12_ERROR("The allocated pointer don't need to reallocate!");
			return;
		}

		*ppDestResource = DX12_NEW_RAW(CResource(m_pDevice, this));

		ID3D12Resource* pResource = nullptr;

		auto allocaInfo = m_pDevice->GetResourceAllocationInfo(1U, 1U, pDesc);
		UINT64 size = allocaInfo.SizeInBytes;

		FrostAutoLock<FrostMutexFast> threadSafeScope(m_ReadBackHeapThreadSafeScope);
		{
			if (m_ReadBackHeapFreeTable.size())
			{
				for (auto it = m_ReadBackHeapFreeTable.begin(); it != m_ReadBackHeapFreeTable.end(); ++it)
				{
					if ((*it)->GetCapacity() >= size)
					{
						if (SUCCEEDED(m_pDevice->CreatePlacedResource(
							m_ReadBackHeap.GetD3D12Heap(),
							(*it)->GetStartOffset(),
							pDesc,
							D3D12_RESOURCE_STATE_COPY_DEST,
							pOptimizedClearValue,
							IID_PPV_ARGS(&pResource))))
						{
							m_ReadBackHeapLookupTable[pResource] = *it;
							m_ReadBackHeapFreeTable.erase(it);
							break;	
						}
						else
						{
							DX12_ERROR("Could not create placed resource from the read back heap!");
						}
						
					}
				}
			}
			else if (!m_ReadBackHeap.IsOutOfBound(size))
			{
				if (SUCCEEDED(m_pDevice->CreatePlacedResource(
					m_ReadBackHeap.GetD3D12Heap(),
					m_ReadBackHeap.GetCursor(),
					pDesc,
					D3D12_RESOURCE_STATE_COPY_DEST,
					pOptimizedClearValue,
					IID_PPV_ARGS(&pResource))))
				{
					m_ReadBackHeap.SetCursor(m_ReadBackHeap.GetCursor() + size);

					CHeapBlock* block = new CHeapBlock(&m_ReadBackHeap, m_ReadBackHeap.GetCursor(), size);

					m_ReadBackHeapLookupTable[pResource] = block;
				}
				else
				{
					DX12_ERROR("Could not create placed resource from the read back heap!");
				}
			}
			else
			{
				if (FAILED(m_pDevice->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
					D3D12_HEAP_FLAG_NONE,
					pDesc,
					D3D12_RESOURCE_STATE_COPY_DEST,
					pOptimizedClearValue,
					IID_PPV_ARGS(&pResource))))
				{
					DX12_ERROR("Could not create committed resource!");
				}
			}
		}

		static_cast<CResource*>(*ppDestResource)->Init(pResource, D3D12_RESOURCE_STATE_COPY_DEST);

		pResource->Release();
	}

	void CHeapAllocator::InvalidateDefaultHeapResource(ID3D12Resource* pResource)
	{
		FrostAutoLock<FrostMutexFast> threadSafeScope(m_DefaultHeapThreadSafeScope);
		{
			auto iter_res = m_DefaultHeapLookupTable.find(pResource);
			if (iter_res != m_DefaultHeapLookupTable.end())
			{
				m_DefaultHeapFreeTable.push_back(iter_res->second);
				m_DefaultHeapLookupTable.erase(iter_res);
			}
		}
	}

	void CHeapAllocator::InvalidateUploadHeapResource(ID3D12Resource* pResource)
	{
		FrostAutoLock<FrostMutexFast> threadSafeScope(m_UploadHeapThreadSafeScope);
		{
			auto iter_res = m_UploadHeapLookupTable.find(pResource);
			if (iter_res != m_UploadHeapLookupTable.end())
			{
				m_UploadHeapFreeTable.push_back(iter_res->second);
				m_UploadHeapLookupTable.erase(iter_res);
			}
		}
	}

	void CHeapAllocator::InvalidateReadBackHeapResource(ID3D12Resource* pResource)
	{
		FrostAutoLock<FrostMutexFast> threadSafeScope(m_ReadBackHeapThreadSafeScope);
		{
			auto iter_res = m_ReadBackHeapLookupTable.find(pResource);
			if (iter_res != m_ReadBackHeapLookupTable.end())
			{
				m_ReadBackHeapFreeTable.push_back(iter_res->second);
				m_ReadBackHeapLookupTable.erase(iter_res);
			}
		}
	}
#pragma endregion
}