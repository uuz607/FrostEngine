#include"pch.h"
#include"DX12GIAdapter.h"
#include"DX12GIFactory.h"

namespace FrostDX12
{
	/*--------------Interface implemtation--------------*/
	void CDXGIAdapter::AddRef()
	{
		::_InterlockedIncrement(&m_RefCount);
	}

	void CDXGIAdapter::Release()
	{
		long refCount = ::_InterlockedDecrement(&m_RefCount);
		if (!refCount)
		{
			delete this;
		}
	}

	/*--------------CDXGIAdapter implemtation--------------*/
	CDXGIAdapter* CDXGIAdapter::Create(CDXGIFactory* pFactory, UINT Adapter)
	{
		IDXGIAdapter1* pAdapter1;
		pFactory->GetDXGIFactory()->EnumAdapters1(Adapter, &pAdapter1);
		IDXGIAdapter3* pAdapter3;
		pAdapter1->QueryInterface(IID_PPV_ARGS(&pAdapter3));

		pAdapter1->Release();

		return pAdapter3 ? DX12_NEW_RAW(CDXGIAdapter(pFactory, Adapter, pAdapter3)) : nullptr;
	}

	CDXGIAdapter::CDXGIAdapter(CDXGIFactory* pFactory, UINT Adapter, IDXGIAdapter3* pAdapter) :
		m_pFactory(pFactory),
		m_pAdapter(pAdapter),
		m_NodeMask(Adapter),
		m_RefCount(0)
	{
		
	}

	CDXGIAdapter::~CDXGIAdapter()
	{

	}
}