#pragma once

#include"DX12\Interface\FrostRenderDXGI.h"
#include"DX12Base.h"

namespace FrostDX12
{
	class CDXGIFactory : public IFrostFactory
	{
	public:
		virtual void AddRef()  override;
		virtual void Release() override;

		bool EnumAdapter(UINT Adapter, IFrostAdapter** ppHardwareAdapter) override;

		void CreateSwapChain(
			IFrostDevice* pDevice,
			HWND hWnd,
			DXGI_SWAP_CHAIN_DESC1* pDesc1,
			const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
			IDXGIOutput *pRestrictToOutput,
			IFrostSwapChain** ppSwapChain) override;

	public:
		static CDXGIFactory* Create();

		~CDXGIFactory() noexcept;

		IDXGIFactory4* GetDXGIFactory() const
		{
			return m_pFactory;
		}

	private:
		CDXGIFactory(IDXGIFactory4* pDXGIFactory4);

		DX12_PTR(IDXGIFactory4) m_pFactory;

		volatile long m_RefCount;
	};
}