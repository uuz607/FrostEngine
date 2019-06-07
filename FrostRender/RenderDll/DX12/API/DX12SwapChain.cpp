#include"pch.h"
#include"DX12SwapChain.h"
#include"DX12Resource.h"

namespace FrostDX12
{
	/*---------------Interface implemtation---------------*/
	void CSwapChain::AddRef()
	{
		::_InterlockedIncrement(&m_RefCount);
	}

	void CSwapChain::Release()
	{
		long refCount = ::_InterlockedDecrement(&m_RefCount);
		if (!refCount)
		{
			delete this;
		}
	}

	UINT CSwapChain::GetCurrentBackBufferIndex() const
	{
		if (!m_bChangedBackBufferIndex)
			return m_CurrentBackBufferIndex;

		m_AsyncQueue->FlushNextPresent();
		DX12_ASSERT(!IsPresentScheduled(), "Flush didn't dry out all outstanding Present() calls!");

		m_CurrentBackBufferIndex = m_pDXGISwapChain->GetCurrentBackBufferIndex();

		m_bChangedBackBufferIndex = false;

		return m_CurrentBackBufferIndex;
	}

	void CSwapChain::GetBuffer(UINT Index, IFrostResource** ppResource)
	{
		if (*ppResource)
		{
			DX12_ERROR("GetBuffer() requires the second parameter to be not null!");

			return;
		}

		*ppResource = static_cast<IFrostResource*>(&m_BackBuffers[Index]);
	}

	void CSwapChain::Present(UINT syncInterval, UINT flags)
	{
		m_AsyncQueue->Present(m_pDXGISwapChain, &m_PresentResult, syncInterval, flags, &m_SwapChainDesc, GetCurrentBackBufferIndex());
		m_bChangedBackBufferIndex = true;
	}

	/*---------------CSwapChain implemtation---------------*/
	CSwapChain* CSwapChain::Create(
		IDXGIFactory4* pFactory,
		CCommandListPool* pCmdListPool,
		HWND hWnd,
		DXGI_SWAP_CHAIN_DESC1* pDesc1,
		const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
		IDXGIOutput *pRestrictToOutput)
	{
		IDXGISwapChain1*	pSwapChain1 = nullptr;
		IDXGISwapChain3*    pSwapChain3 = nullptr;
		ID3D12CommandQueue* pCommandQueue = pCmdListPool->GetD3D12CommandQueue();

		// If discard isn't implemented/supported/fails, try the newer swap-types
#ifdef __dxgi1_4_h__
		if (pDesc1->SwapEffect == DXGI_SWAP_EFFECT_SEQUENTIAL)
		{
			// - flip_sequentially is win 8
			pDesc1->SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
			pDesc1->BufferCount =  std::max<>(2U, pDesc1->BufferCount);
		}
		else if (pDesc1->SwapEffect == DXGI_SWAP_EFFECT_DISCARD)
		{
			// - flip_discard is win10
			pDesc1->SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			pDesc1->BufferCount = std::max<>(2U, pDesc1->BufferCount);
		}
#endif 
		CSwapChain* result = nullptr;

		if (SUCCEEDED(pFactory->CreateSwapChainForHwnd(
			pCommandQueue, hWnd, pDesc1,
			pFullscreenDesc, pRestrictToOutput, &pSwapChain1)))
		{
			if (SUCCEEDED(pSwapChain1->QueryInterface(IID_PPV_ARGS(&pSwapChain3))))
			{
				result = DX12_NEW_RAW(CSwapChain(pCmdListPool, pSwapChain3, pDesc1));
				pSwapChain3->Release();
			}
		}
		
		pSwapChain1->Release();
	
		return result;
	}

	CSwapChain::CSwapChain(CCommandListPool* pCmdListPool, IDXGISwapChain3* pSwapChain3, DXGI_SWAP_CHAIN_DESC1* pDesc) :
		m_CommandQueue(pCmdListPool),
		m_AsyncQueue(pCmdListPool->GetAsyncCommandQueue()),
		m_SwapChainDesc(*pDesc),
		m_pDXGISwapChain(pSwapChain3),
		m_PresentResult(S_OK),
		m_RefCount(0),
		m_CurrentBackBufferIndex(0),
		m_bChangedBackBufferIndex(true)
	{
		AcquireBuffers();
	}

	CSwapChain::~CSwapChain()
	{
		m_BackBuffers.clear();
	}

	HRESULT CSwapChain::ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters)
	{
		m_bChangedBackBufferIndex = true;

		return m_pDXGISwapChain->ResizeTarget(pNewTargetParameters);
	}

	HRESULT CSwapChain::ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
	{
		m_bChangedBackBufferIndex = true;

		// DXGI ERROR: IDXGISwapChain::ResizeBuffers: Cannot add or remove the DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING flag using ResizeBuffers. 
		m_SwapChainDesc.BufferCount = BufferCount ? BufferCount : m_SwapChainDesc.BufferCount;
		m_SwapChainDesc.Width = Width ? Width : m_SwapChainDesc.Width;
		m_SwapChainDesc.Height = Height ? Height : m_SwapChainDesc.Height;
		m_SwapChainDesc.Format = NewFormat != DXGI_FORMAT_UNKNOWN ? NewFormat : m_SwapChainDesc.Format;
	#ifdef __dxgi1_3_h__
		m_SwapChainDesc.Flags = (m_SwapChainDesc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) | (SwapChainFlags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
	#endif 

		return m_pDXGISwapChain->ResizeBuffers(
			m_SwapChainDesc.BufferCount,
			m_SwapChainDesc.Width,
			m_SwapChainDesc.Height,
			m_SwapChainDesc.Format,
			m_SwapChainDesc.Flags);
	}

	void CSwapChain::AcquireBuffers()
	{
		m_BackBuffers.reserve(m_SwapChainDesc.BufferCount);

		for (UINT i = 0; i < m_SwapChainDesc.BufferCount; ++i)
		{
			ID3D12Resource* pResource = nullptr;
			if (FAILED(m_pDXGISwapChain->GetBuffer(i,IID_PPV_ARGS(&pResource))))
			{
				DX12_ERROR("Fail to get buffer");
				continue;
			}

			CResource backBuffer(m_CommandQueue->GetDevice());
			backBuffer.Init(pResource, D3D12_RESOURCE_STATE_PRESENT);
			backBuffer.SetDX12SwapChain(this);

			m_BackBuffers.emplace_back(std::move(backBuffer));

			pResource->Release();
		}
	}
}