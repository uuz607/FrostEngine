#pragma once

#include"FrostRender.h"

struct IFrostDevice : public IRefCounted
{
	virtual void CreateDescriptorAllocator(
		const DESCRIPTOR_ALLOCATOR_DESC& AllocatorDesc, IFrostDescriptorAllocator** ppIDescAllocator) = 0;

	virtual void CreateHeapAllocator(
		const HEAP_ALLOCATOR_DESC& AllocaDesc, IFrostHeapAllocator** ppIHeapAlloca) = 0;

	virtual void CreateDefaultHeapResource(
		const D3D12_RESOURCE_DESC* pDesc,
		D3D12_RESOURCE_STATES InitialState,
		const D3D12_CLEAR_VALUE* pOptimizedClearValue,
		IFrostHeapAllocator* pIHeapAllocator,
		IFrostResource** ppIDestResource) = 0;

	virtual void CreateUploadHeapResource(
		const D3D12_RESOURCE_DESC* pDesc,
		const D3D12_CLEAR_VALUE* pOptimizedClearValue,
		IFrostHeapAllocator* pIHeapAllocator,
		IFrostResource** ppIDestResource) = 0;

	virtual void CreateReadBackHeap(
		const D3D12_RESOURCE_DESC* pDesc,
		const D3D12_CLEAR_VALUE* pOptimizedClearValue,
		IFrostHeapAllocator* pIHeapAllocator,
		IFrostResource** ppIDestResource) = 0;

	virtual void CreateSampler(
		const D3D12_SAMPLER_DESC* pDesc,
		IFrostDescriptorAllocator* pIDescAllocator,
		IFrostSamplerState** ppISamplerState) = 0;

	virtual void CreateVertexBufferView(
		const D3D12_VERTEX_BUFFER_VIEW& ViewDesc,
		IFrostResource* pResource,
		IFrostView** ppIView) = 0;

	virtual void CreateIndexBufferView(
		const D3D12_INDEX_BUFFER_VIEW& ViewDesc,
		IFrostResource* pResource,
		IFrostView** ppIView) = 0;

	virtual void CreateConstantBufferView(
		const D3D12_CONSTANT_BUFFER_VIEW_DESC*  pDesc,
		IFrostDescriptorAllocator* pDescAllocator,
		IFrostResource* pIResource,
		IFrostView** ppIView) = 0;

	virtual void CreateShaderResourceView(
		const D3D12_SHADER_RESOURCE_VIEW_DESC*  pDesc,
		IFrostDescriptorAllocator* pDescAllocator,
		IFrostResource* pIResource,
		IFrostView** ppIView) = 0;

	virtual void CreateUnorderedAccessView(
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc,
		IFrostDescriptorAllocator* pDescAllocator,
		IFrostResource* pIResource,
		IFrostView** ppIView) = 0;

	virtual void CreateDepthStencilView(
		const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc,
		IFrostDescriptorAllocator* pDescAllocator,
		IFrostResource* pIResource,
		IFrostView** ppIView) = 0;

	virtual void CreateRenderTargetView(
		const D3D12_RENDER_TARGET_VIEW_DESC* pDesc,
		IFrostDescriptorAllocator* pDescAllocator,
		IFrostResource* pIResource,
		IFrostView** ppIView) = 0;

	virtual void CreateRootSignature(
		const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignatureDesc,
		bool bGfx,
		DX12_PTR(IFrostRootSignature)& ppIRootSignature) = 0;

	virtual void CreateGraphicsPipelineState(
		const D3D12_GRAPHICS_PIPELINE_STATE_DESC& GfxPSODesc,
		DX12_PTR(IFrostPSO)& pIPSO) = 0;

	virtual void CreateComputePipelineState(
		const D3D12_COMPUTE_PIPELINE_STATE_DESC& ComputePSODesc,
		DX12_PTR(IFrostPSO)& pIPSO) = 0;

	virtual void CreateCommandList(
		int queueIndex,
		DX12_PTR(IFrostCommandList)& pCmdList) = 0;

	virtual void ExecuteCommandList(
		DX12_PTR(IFrostCommandList)& pCmdList,
		bool bWait = false) = 0;

	virtual void HoldBack(
		int queueIndex,
		IFrostCommandList* pICmdList) = 0;
};

struct IFrostAdapter : public IRefCounted
{

};

struct IFrostSwapChain : public IRefCounted
{
	virtual UINT GetCurrentBackBufferIndex() const = 0;

	virtual void GetBuffer(UINT Index, IFrostResource** ppResource) = 0;

	virtual void Present(UINT SyncInterval, UINT Flags) = 0;
};

struct IFrostFactory : public IRefCounted
{
	virtual bool EnumAdapter(UINT Adapter, IFrostAdapter** ppHardwareAdapter) = 0;

	virtual void CreateSwapChain(
		IFrostDevice* pDevice,
		HWND hWnd,
		DXGI_SWAP_CHAIN_DESC1* pDesc1,
		const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
		IDXGIOutput *pRestrictToOutput,
		IFrostSwapChain** ppSwapChain) = 0;
};

extern void __declspec(dllexport)CreateFrostFactory(IFrostFactory** ppFactory);

extern void __declspec(dllexport)CreateFrostDevice(IFrostAdapter* pFrostAdapter, IFrostDevice** ppDevice);

