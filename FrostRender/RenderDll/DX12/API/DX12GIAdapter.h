#pragma once

#include"DX12\Interface\FrostRenderDXGI.h"
#include"DX12Base.h"

namespace FrostDX12
{
	class CDXGIFactory;

	class CDXGIAdapter : public IFrostAdapter
	{
	public:
		virtual void AddRef()  override;
		virtual void Release() override;

	public:
		static CDXGIAdapter* Create(CDXGIFactory* pFactory, UINT Adapter);

		~CDXGIAdapter()noexcept;

		IDXGIAdapter3* GetDXGIAdapter() const
		{
			return m_pAdapter;
		}

		UINT GetNodeMask() const
		{
			return m_NodeMask;
		}

	private:
		CDXGIAdapter(CDXGIFactory* pFactory, UINT Adapter, IDXGIAdapter3* pAdapter);

		CDXGIFactory* m_pFactory;
		DX12_PTR(IDXGIAdapter3) m_pAdapter;

		UINT m_NodeMask;

		volatile long m_RefCount;
	};
}