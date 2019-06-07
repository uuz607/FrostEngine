#include"pch.h"
#include"DX12RootSignature.h"

namespace FrostDX12
{
#pragma region CRootSignature Implemtation

	/*--------------Interface implemtation--------------*/
	void CRootSignature::AddRef()
	{
		::_InterlockedIncrement(&m_RefCount);
	}

	void CRootSignature::Release()
	{
		long refCount = ::_InterlockedDecrement(&m_RefCount);
		if (!refCount)
		{
			delete this;
		}
	}

	/*--------------CRootSignature implemtation--------------*/
	CRootSignature::CRootSignature(ID3D12Device* pDevice, UINT NodeMask) :
		m_pDevice(pDevice),
		m_NodeMask(NodeMask),
		m_RefCount(0)
	{

	}

	CRootSignature::~CRootSignature()
	{

	}

	bool CRootSignature::Serialize(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignatureDesc)
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if (FAILED(m_pDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		{
			ID3DBlob* pOutBlob = nullptr;
			ID3DBlob* pErrorBlob = nullptr;

			if (FAILED(::D3DX12SerializeVersionedRootSignature(
				pRootSignatureDesc, 
				featureData.HighestVersion, 
				&pOutBlob, 
				&pErrorBlob)))
			{
				DX12_ERROR("Could not serialize root signature!");
				return false;
			}
			
			if (FAILED(m_pDevice->CreateRootSignature(
				m_NodeMask, 
				pOutBlob->GetBufferPointer(),
				pOutBlob->GetBufferSize(), 
				IID_PPV_ARGS(&m_pRootSignature))))
			{
				DX12_ERROR("Could not create root signature!");
				return false;
			}

			if (pOutBlob)
			{
				pOutBlob->Release();
			}

			if (pErrorBlob)
			{
				pErrorBlob->Release();
			}
		}

		return true;
	}
#pragma endregion

#pragma region CRootSignatureCache

	CRootSignatureCache::CRootSignatureCache(ID3D12Device* pDevice) :
		m_pDevice(pDevice)
	{

	}

	CRootSignatureCache::~CRootSignatureCache()
	{

	}

	void CRootSignatureCache::GetOrCreateRootSignature(
		const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignatureDesc,
		bool bGfx,
		UINT NodeMask,
		DX12_PTR(IFrostRootSignature)& pIRootSignature)
	{
		// LSB filled marks compute pipeline states, in case graphics and compute hashes collide
		Hash hash = (bGfx ? 1 : ~1) & ComputeSmallHash<sizeof(D3D12_VERSIONED_ROOT_SIGNATURE_DESC)>(pRootSignatureDesc);
		auto iter = m_RootSignatureMap.find(hash);
		
		if (iter != m_RootSignatureMap.end())
		{
			pIRootSignature = iter->second;
			return;
		}

		pIRootSignature = new CRootSignature(m_pDevice, NodeMask);

		if (!static_cast<CRootSignature*>(pIRootSignature.get())->Serialize(pRootSignatureDesc))
		{
			DX12_ERROR("Could not create root signature!");

			pIRootSignature = nullptr;

			return;
		}
	
		m_RootSignatureMap[hash] = pIRootSignature;
	}
#pragma endregion
}

