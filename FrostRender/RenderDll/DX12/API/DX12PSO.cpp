#include"pch.h"
#include"DX12PSO.h"

namespace FrostDX12
{
#pragma region CPSO implementation

	/*--------------Interface implemtation--------------*/
	void CPSO::AddRef()
	{
		::_InterlockedIncrement(&m_RefCount);
	}

	void CPSO::Release()
	{
		long refCount = ::_InterlockedDecrement(&m_RefCount);
		if (!refCount)
		{
			delete this;
		}
	}

	/*--------------CPSO implemtation--------------*/
	CPSO::CPSO():
		m_pD3D12PipelineState(nullptr),
		m_RefCount(0)
	{

	}

	CPSO::~CPSO()
	{

	}

#pragma endregion

#pragma region CGraphicsPSO based on CPSO

	CGraphicsPSO::CGraphicsPSO() : CPSO() 
	{

	}

	CGraphicsPSO::~CGraphicsPSO()
	{

	}

	bool CGraphicsPSO::Init(
		ID3D12Device* pDevice, 
		const D3D12_GRAPHICS_PIPELINE_STATE_DESC& GfxPSODesc)
	{
		m_PipelineStateDesc = GfxPSODesc;

		ID3D12PipelineState* pipelineState = nullptr;
		HRESULT result = pDevice->CreateGraphicsPipelineState(&m_PipelineStateDesc, IID_PPV_ARGS(&pipelineState));

		if (FAILED(result))
		{
			DX12_ERROR("Could not create graphics pipeline state!");
			return false;
		}

		CPSO::Init(pipelineState);
		pipelineState->Release();

		return true;
	}

	const D3D12_GRAPHICS_PIPELINE_STATE_DESC& CGraphicsPSO::GetGraphicsPipelineStateDesc() const
	{
		return m_PipelineStateDesc;
	}
#pragma endregion

#pragma region CComputePSO based on CPSO

	CComputePSO::CComputePSO() : CPSO()
	{

	}

	CComputePSO::~CComputePSO()
	{

	}

	bool CComputePSO::Init(
		ID3D12Device* pDevice,
		const D3D12_COMPUTE_PIPELINE_STATE_DESC& ComputePSODesc)
	{
		m_PipelineStateDesc = ComputePSODesc;

		ID3D12PipelineState* pipelineState = nullptr;
		HRESULT result = pDevice->CreateComputePipelineState(&m_PipelineStateDesc, IID_PPV_ARGS(&pipelineState));

		if (FAILED(result))
		{
			DX12_ERROR("Could not create graphics pipeline state!");
			return false;
		}

		CPSO::Init(pipelineState);
		pipelineState->Release();

		return true;
	}

	const D3D12_COMPUTE_PIPELINE_STATE_DESC& CComputePSO::GetComputePipelineStateDesc() const
	{
		return m_PipelineStateDesc;
	}
#pragma endregion

#pragma region CPSOCache implementation

	CPSOCache::CPSOCache(ID3D12Device* pDevice) :
		m_pDevice(pDevice)
	{

	}

	CPSOCache::~CPSOCache()
	{

	}

	void CPSOCache::GetOrCreatePSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& GfxPSODesc, DX12_PTR(IFrostPSO)& pPSO)
	{
		DX12_ASSERT(&GfxPSODesc, "The 'GfxPSODesc' is not allowed to keep null!");

		Hash hash = ComputeSmallHash<sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC)>(&GfxPSODesc);

		auto iter = m_PSOMap.find(hash);

		if (iter != m_PSOMap.end())
		{
			pPSO = iter->second;

			return;
		}

		pPSO = new CGraphicsPSO();
		if (!(static_cast<CGraphicsPSO*>(pPSO.get())->Init(m_pDevice, GfxPSODesc)))
		{
			DX12_ERROR("Could not create Graphics PSO!");

			pPSO = nullptr;

			return;
		}

		m_PSOMap[hash] = pPSO;
	}

	void CPSOCache::GetOrCreatePSO(const D3D12_COMPUTE_PIPELINE_STATE_DESC& ComputePSODesc, DX12_PTR(IFrostPSO)& pPSO)
	{
		DX12_ASSERT(&ComputePSODesc, "The 'GfxPSODesc' is not allowed to keep null!");

		Hash hash = ComputeSmallHash<sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC)>(&ComputePSODesc);

		auto iter = m_PSOMap.find(hash);

		if (iter != m_PSOMap.end())
		{
			pPSO = iter->second;

			return;
		}

		pPSO = new CComputePSO();
		if (!(static_cast<CComputePSO*>(pPSO.get())->Init(m_pDevice, ComputePSODesc)))
		{
			DX12_ERROR("Could not create Graphics PSO!");

			pPSO = nullptr;

			return;
		}

		m_PSOMap[hash] = pPSO;
	}

#pragma endregion
}