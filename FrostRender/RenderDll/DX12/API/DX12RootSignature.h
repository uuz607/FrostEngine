#pragma once

#include"DX12\Interface\FrostRender.h"
#include"DX12Base.h"

namespace FrostDX12
{
	class CRootSignature : public IFrostRootSignature
	{
	public:
		virtual void AddRef()  override;
		virtual void Release() override;

		ID3D12RootSignature* GetD3D12RootSignature() const override
		{
			return m_pRootSignature;
		}

	public:
		CRootSignature(ID3D12Device* pDevice, UINT NodeMask);
		~CRootSignature() noexcept;

		bool Serialize(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignatureDesc);

	private:
		DX12_PTR(ID3D12RootSignature) m_pRootSignature;

		ID3D12Device* m_pDevice;

		UINT m_NodeMask;

		volatile long m_RefCount;
	};

	class CRootSignatureCache
	{
	public:
		CRootSignatureCache(ID3D12Device* pDevice);
		~CRootSignatureCache();

		void GetOrCreateRootSignature(
			const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignatureDesc, 
			bool bGfx,
			UINT NodeMask,
			DX12_PTR(IFrostRootSignature)& pIRootSignature);

	private:
		ID3D12Device* m_pDevice;

		typedef std::unordered_map<Hash, DX12_PTR(IFrostRootSignature)> RootSignatureMap;
		RootSignatureMap m_RootSignatureMap;
	};
}