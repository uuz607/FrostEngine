
#include"pch.h"
#include"DX12GIFactory.h"
#include"DX12GIAdapter.h"
#include"DX12SwapChain.h"
#include"DX12Device.h"

namespace FrostDX12
{
	/*--------------Interface implemtation--------------*/
	void  CDXGIFactory::AddRef() 
	{ 
		::_InterlockedIncrement(&m_RefCount); 
	}

	void  CDXGIFactory::Release() 
	{
		long refCount = ::_InterlockedDecrement(&m_RefCount);
		if (!refCount)
		{
			delete this;
		}
	}

	bool CDXGIFactory::EnumAdapter(UINT Adapter, IFrostAdapter** ppHardwareAdapter)
	{
		if (*ppHardwareAdapter)
		{
			DX12_ERROR("The IFrostAdapter don't need to reallocate!");

			return false;
		}

		*ppHardwareAdapter = CDXGIAdapter::Create(this, Adapter);

		return true;
	}

	void CDXGIFactory::CreateSwapChain(
		IFrostDevice* pDevice,
		HWND hWnd,
		DXGI_SWAP_CHAIN_DESC1* pDesc1,
		const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
		IDXGIOutput *pRestrictToOutput,
		IFrostSwapChain** ppSwapChain)
	{
		if (*ppSwapChain)
		{
			DX12_ERROR("The IFrostSwapChain don't need to reallocate!");

			return;
		}

		if (!GetDXGIFactory())
		{
			DX12_ERROR("The IDXGIFactory cannot be null!");

			return;
		}

		*ppSwapChain = CSwapChain::Create(
			GetDXGIFactory(),
			static_cast<CDevice*>(pDevice)->GetCmdListPool(CMDQUEUE_GRAPHICS),
			hWnd,
			pDesc1,
			pFullscreenDesc,
			pRestrictToOutput);
	}
	/*--------------CDXGIFactory implemtation--------------*/
	CDXGIFactory* CDXGIFactory::Create()
	{
		IDXGIFactory4* pDXGIFactory4 = nullptr;

		UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
		ID3D12Debug* pDebug = nullptr;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug))))
		{
			pDebug->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
#endif

	#ifdef FROST_PLATFORM_DESKTOP
		if (FAILED(::CreateDXGIFactory2(dxgiFactoryFlags,IID_PPV_ARGS(&pDXGIFactory4))))
	#endif
		{
			DX12_ASSERT(0, "Failed to create underlying DXGI factory!");

			return nullptr;
		}

		return DX12_NEW_RAW(CDXGIFactory(pDXGIFactory4));
	}


	CDXGIFactory::CDXGIFactory(IDXGIFactory4* pDXGIFactory4):
		m_pFactory(pDXGIFactory4),
		m_RefCount(0)
	{

	}

	CDXGIFactory::~CDXGIFactory()
	{

	}
}