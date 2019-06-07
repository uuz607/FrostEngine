#pragma once

#include"DX12\Interface\FrostRenderDXGI.h"
#include"DX12Base.h"
#include"DX12CommandList.h"
#include"DX12Resource.h"

namespace FrostDX12
{
	class CSwapChain : public IFrostSwapChain
	{
	public:
		void AddRef()  override;
		void Release() override;

		// Get the index of the back-buffer which is to be used for the next Present().
		// Cache the value for multiple calls between Present()s, as the cost of GetCurrentBackBufferIndex() could be high.
		UINT GetCurrentBackBufferIndex() const override;

		void GetBuffer(UINT Index, IFrostResource** ppResource) override;

		// Run Present() asynchronuously, which means the next back-buffer index is not queryable within this function.
		// Instead defer acquiring the next index to the next call of GetCurrentBackbufferIndex().
		void Present(UINT SyncInterval, UINT Flags) override;

	public:
		static CSwapChain* Create(
			IDXGIFactory4* pFactory,
			CCommandListPool* pCmdListPool,
			HWND hWnd, 
			DXGI_SWAP_CHAIN_DESC1* pDesc1,
			const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
			IDXGIOutput *pRestrictToOutput);

	public:
		IDXGISwapChain3* GetDXGISwapChain() const
		{
			return m_pDXGISwapChain;
		}

		UINT GetBackBufferCount()
		{
			return m_SwapChainDesc.BufferCount;
		}

		CResource& GetBackBuffer(UINT index)
		{
			return m_BackBuffers[index];
		}

		CResource& GetCurrentBackBuffer()
		{
			return m_BackBuffers[GetCurrentBackBufferIndex()];
		}

		bool IsPresentScheduled() const
		{
			return m_AsyncQueue->GetQueuedFramesCount() > 0;
		}

		HRESULT GetLastPresentReturnValue()
		{
			return m_PresentResult;
		}

		HRESULT ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters);
		HRESULT ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

		void    AcquireBuffers();

	private:
		CSwapChain(CCommandListPool* pCmdListPool, IDXGISwapChain3* pSwapChain1, DXGI_SWAP_CHAIN_DESC1* pDesc1);
		~CSwapChain() noexcept;

		CCommandListPool*	 m_CommandQueue;
		CAsyncCommandQueue*  m_AsyncQueue;
		DXGI_SWAP_CHAIN_DESC1 m_SwapChainDesc;
	
		DX12_PTR(IDXGISwapChain3) m_pDXGISwapChain;

		std::vector<CResource> m_BackBuffers;

		HRESULT	m_PresentResult;

		volatile long m_RefCount;

		mutable UINT m_CurrentBackBufferIndex;
		mutable bool m_bChangedBackBufferIndex;
	};
}