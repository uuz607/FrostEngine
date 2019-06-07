#pragma once

#include"DX12\Interface\FrostRenderDXGI.h"
#include"DX12Base.h"
#include"DX12CommandScheduler.h"
#include"DX12RootSignature.h"

namespace FrostDX12
{	
	class CDXGIAdapter;

	class CDevice : public IFrostDevice
	{
	public:
		virtual inline void AddRef()  override;
		virtual inline void Release() override;

		void CreateDescriptorAllocator(
			const DESCRIPTOR_ALLOCATOR_DESC& AllocatorDesc,
			IFrostDescriptorAllocator** ppDescAllocator) override;

		void CreateHeapAllocator(
			const HEAP_ALLOCATOR_DESC& AllocaDesc,
			IFrostHeapAllocator** ppHeapAlloca) override;

		void CreateDefaultHeapResource(
			const D3D12_RESOURCE_DESC* pDesc, 
			D3D12_RESOURCE_STATES InitialState,
			const D3D12_CLEAR_VALUE* pOptimizedClearValue, 
			IFrostHeapAllocator* pHeapAllocator,
			IFrostResource** ppDestResource) override;

		void CreateUploadHeapResource(
			const D3D12_RESOURCE_DESC* pDesc,
			const D3D12_CLEAR_VALUE* pOptimizedClearValue,
			IFrostHeapAllocator* pHeapAllocator,
			IFrostResource** ppDestResource) override;

		void CreateReadBackHeap(
			const D3D12_RESOURCE_DESC* pDesc,
			const D3D12_CLEAR_VALUE* pOptimizedClearValue,
			IFrostHeapAllocator* pHeapAllocator,
			IFrostResource** ppDestResource) override;

		void CreateSampler(
			const D3D12_SAMPLER_DESC* pDesc,
			IFrostDescriptorAllocator* pDescAllocator,
			IFrostSamplerState** ppSamplerState) override;

		void CreateVertexBufferView(
			const D3D12_VERTEX_BUFFER_VIEW& ViewDesc,
			IFrostResource* pResource,
			IFrostView** ppIView) override;

		void CreateIndexBufferView(
			const D3D12_INDEX_BUFFER_VIEW& ViewDesc,
			IFrostResource* pResource,
			IFrostView** ppIView) override;

		void CreateConstantBufferView(
			const D3D12_CONSTANT_BUFFER_VIEW_DESC*  pDesc,
			IFrostDescriptorAllocator* pDescAllocator,
			IFrostResource* pResource,
			IFrostView** ppView) override;

		void CreateShaderResourceView(
			const D3D12_SHADER_RESOURCE_VIEW_DESC*  pDesc,
			IFrostDescriptorAllocator* pDescAllocator,
			IFrostResource* pResource,
			IFrostView** ppView) override;

		void CreateUnorderedAccessView(
			const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc,
			IFrostDescriptorAllocator* pDescAllocator,
			IFrostResource* pResource,
			IFrostView** ppView) override;

		void CreateDepthStencilView(
			const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc,
			IFrostDescriptorAllocator* pDescAllocator,
			IFrostResource* pResource,
			IFrostView** ppView) override;

		void CreateRenderTargetView(
			const D3D12_RENDER_TARGET_VIEW_DESC* pDesc,
			IFrostDescriptorAllocator* pDescAllocator,
			IFrostResource* pResource,
			IFrostView** ppView) override;

		void CreateRootSignature(
			const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignatureDesc,
			bool bGfx,
			DX12_PTR(IFrostRootSignature)& pIRootSignature) override;

		void CreateGraphicsPipelineState(
			const D3D12_GRAPHICS_PIPELINE_STATE_DESC& GfxPSODesc,
			DX12_PTR(IFrostPSO)& pIPSO) override;

		void CreateComputePipelineState(
			const D3D12_COMPUTE_PIPELINE_STATE_DESC& ComputePSODesc,
			DX12_PTR(IFrostPSO)& pIPSO) override;

		void CreateCommandList(
			int queueIndex,
			DX12_PTR(IFrostCommandList)& pCmdList) override;

		void ExecuteCommandList(
			DX12_PTR(IFrostCommandList)& pCmdList,
			bool bWait = false) override;

		void HoldBack(int queueIndex, IFrostCommandList* pICmdList) override;

	public:
		static CDevice* Create(IFrostAdapter* pAdapter);

		ID3D12Device* GetD3D12Device() const
		{
			return m_pDevice;
		}

		D3D_FEATURE_LEVEL GetFeatureLevel()const
		{
			return m_FeatureLevel;
		}

		CCommandListPool* GetCmdListPool(int queueIndex)
		{
			return m_CmdScheduler.GetCommandListPool(queueIndex);
		}

		UINT GetNodeCount()const
		{
			return m_NodeCount;
		}

		UINT GetNodeMask() const
		{
			return m_NodeMask;
		}

		CPSOCache* GetPSOCache()
		{
			return &m_PSOCache;
		}

		CRootSignatureCache* GetRootSignatureCache()
		{
			return &m_RootSignatureCache;
		}

		inline HRESULT STDMETHODCALLTYPE CheckFeatureSupport(
			D3D12_FEATURE Feature,
			void* pFeatureSupportData,
			UINT FeatureSupportDataSize);
	
	private:
		CDevice(ID3D12Device* pD3D12Device, D3D_FEATURE_LEVEL FeatureLevel, UINT NodeCount, UINT NodeMask);
		~CDevice();

		CDevice(const CDevice&) = delete;
		CDevice& operator=(const CDevice&) = delete;
	
		DX12_PTR(ID3D12Device) m_pDevice;
		D3D_FEATURE_LEVEL      m_FeatureLevel;

		UINT m_NodeCount;
		UINT m_NodeMask;

		CCommandScheduler	 m_CmdScheduler;
		CPSOCache			 m_PSOCache;
		CRootSignatureCache  m_RootSignatureCache;

		volatile long m_RefCount;
	};
}