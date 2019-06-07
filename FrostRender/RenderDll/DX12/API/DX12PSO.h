#pragma once

#include"DX12\Interface\FrostRender.h"
#include"DX12Base.h"

namespace FrostDX12
{
	class CPSOCache;

	class CPSO : public IFrostPSO
	{
	public:
		virtual void AddRef()  override;
		virtual void Release() override;

	public:
		CPSO();
		~CPSO() noexcept; 

		void Init(ID3D12PipelineState* pD3D12PipelineState)
		{
			m_pD3D12PipelineState = pD3D12PipelineState;
		}

		ID3D12PipelineState* GetD3D12PipelineState() const
		{
			return m_pD3D12PipelineState;
		}

	protected:
		DX12_PTR(ID3D12PipelineState) m_pD3D12PipelineState;

		volatile long m_RefCount;
	};

	class CGraphicsPSO : public CPSO
	{
	public:
		CGraphicsPSO();
		~CGraphicsPSO() noexcept;

		bool Init(ID3D12Device* pDevice, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& GfxPSODesc);

		const D3D12_GRAPHICS_PIPELINE_STATE_DESC& GetGraphicsPipelineStateDesc() const;

	private:
		D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PipelineStateDesc;
	};

	class CComputePSO : public CPSO
	{
	public:
		CComputePSO();
		~CComputePSO() noexcept;

		bool Init(ID3D12Device* pDevice, const D3D12_COMPUTE_PIPELINE_STATE_DESC& ComputePSODesc);

		const D3D12_COMPUTE_PIPELINE_STATE_DESC& GetComputePipelineStateDesc() const;

	private:
		D3D12_COMPUTE_PIPELINE_STATE_DESC m_PipelineStateDesc;
	};

	class CPSOCache
	{
	public:
		CPSOCache(ID3D12Device* pDevice);
		~CPSOCache() noexcept;

		void GetOrCreatePSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& GfxPSODesc,	  DX12_PTR(IFrostPSO)& pPSO);
		void GetOrCreatePSO(const D3D12_COMPUTE_PIPELINE_STATE_DESC&  ComputePSODesc, DX12_PTR(IFrostPSO)& pPSO);

	private:
		ID3D12Device* m_pDevice;

		std::unordered_map<Hash, DX12_PTR(IFrostPSO)> m_PSOMap;
	};
}