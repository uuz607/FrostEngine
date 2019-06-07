#pragma once

#include<smartptr.h>
#define DX12_PTR(T)     _smart_ptr<T>

#define CMDQUEUE_GRAPHICS 0
#define CMDQUEUE_COMPUTE  1
#define CMDQUEUE_COPY     2
#define CMDQUEUE_NUM      3

struct IRefCounted
{
	virtual ~IRefCounted() {}
	virtual void AddRef()  = 0;
	virtual void Release() = 0;
};

struct IFrostResource : public IRefCounted
{
	virtual bool MappedWriteToSubresource(
		UINT Subresource, const D3D12_RANGE* pWrittenRange, const void* pInData) = 0;
	virtual bool MappedReadFromSubresource(
		UINT Subresource, const D3D12_RANGE* pReadRange, void* pOutData) = 0;

	virtual HRESULT STDMETHODCALLTYPE Map(
		UINT Subresource,
		_In_opt_  const D3D12_RANGE *pReadRange,
		_Outptr_opt_result_bytebuffer_(_Inexpressible_("Dependent on resource"))  void **ppData) = 0;

	virtual ID3D12Resource*  GetD3D12Resource() const = 0;

	virtual D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const = 0;
};

struct IFrostView : public IRefCounted
{
	virtual void Invalidate() = 0;
};

struct IFrostSamplerState : public IRefCounted
{
	virtual void Invalidate() = 0;
};

struct DESCRIPTOR_ALLOCATOR_DESC
{
	D3D12_DESCRIPTOR_HEAP_DESC SamplerDesc;
	D3D12_DESCRIPTOR_HEAP_DESC CbvSrvUavDesc;
	D3D12_DESCRIPTOR_HEAP_DESC DepthStencilDesc;
	D3D12_DESCRIPTOR_HEAP_DESC RenderTargetDesc;
};

struct IFrostDescriptorAllocator : public IRefCounted
{
	virtual void OffsetInSamplerDescriptors(INT offsetInDescriptors) = 0;
	virtual void OffsetInCbvSrvUavDescriptors(INT offsetInDescriptors) = 0;
	virtual void OffsetInDsvDescriptors(INT offsetInDescriptors) = 0;
	virtual void OffsetInRtvDescriptors(INT offsetInDescriptors) = 0;
};

struct HEAP_ALLOCATOR_DESC
{
	D3D12_HEAP_DESC DefaultHeapDesc;
	D3D12_HEAP_DESC UploadHeapDesc;
	D3D12_HEAP_DESC ReadBackHeapDesc;
};

struct IFrostHeapAllocator : public IRefCounted
{

};

struct IFrostPSO : public IRefCounted
{

};

struct IFrostRootSignature : public IRefCounted
{
	virtual ID3D12RootSignature* GetD3D12RootSignature()const = 0;
};

struct IFrostCommandList : public IRefCounted
{
	virtual UINT64 GetCurrentFenceValue() const = 0;

	virtual UINT64 SignalFenceOnGPU() = 0;
	virtual UINT64 SignalFenceOnCPU() = 0;

	virtual void WaitForFinishOnGPU(
		const UINT64 FenceValue,
		const int id) const = 0;

	virtual void WaitForFinishOnCPU() const = 0;

	virtual void ResourceBarrier(
		IFrostResource* pResource,
		const D3D12_RESOURCE_STATES& DesiredState,
		UINT SubResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		const D3D12_RESOURCE_BARRIER_FLAGS& Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) = 0;

	virtual void AliasBarrier(
		IFrostResource* pResourceBefore,
		IFrostResource* pResourceAfter) = 0;

	virtual void SetVertexBuffers(
		UINT StartSlot, 
		UINT NumViews, 
		IFrostView** ppViews) = 0;

	virtual void SetIndexBuffer(IFrostView *pView) = 0;

	virtual void SetPipelineState(IFrostPSO* pso) = 0;

	virtual void SetGraphicsRootSignature(IFrostRootSignature* pRootSignature) = 0;

	virtual void SetComputeRootSignature(IFrostRootSignature* pRootSignature) = 0;

	virtual void SetGraphicsDescriptorTable(
		UINT RootParameterIndex,
		IFrostView* pView) = 0;

	virtual void SetGraphicsDescriptorTable(
		UINT RootParameterIndex, 
		IFrostSamplerState* pSampler) = 0;

	virtual void SetRenderTargets(
		UINT NumRenderTargetDescriptors,
		IFrostView** ppRenderTargets,
		BOOL RTsSingleHandleToDescriptorRange,
		IFrostView* pDepthStencil) = 0;

	virtual void SetStencilRef(UINT StencilRef) = 0;

	virtual void SetViewports(UINT NumViewports, const D3D12_VIEWPORT* pViewports) = 0;

	virtual void SetScissorRects(UINT NumRects, const D3D12_RECT* pRects) = 0;

	virtual void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology) = 0;

	virtual void SetDescriptorHeaps(IFrostDescriptorAllocator* heaps) = 0;

	virtual void ClearDepthStencilView(
		IFrostView* pDepthStencilView, 
		D3D12_CLEAR_FLAGS ClearFlags, 
		float depthValue,
		UINT StencilValue, 
		UINT NumRects = 0U, 
		const D3D12_RECT* pRect = nullptr) = 0;

	virtual void ClearRenderTargetView(
		IFrostView* pRenderTargetView, 
		const FLOAT rgba[4], 
		UINT NumRects = 0U, 
		const D3D12_RECT* pRects = nullptr) = 0;

	virtual void ClearUnorderedAccessView(
		IFrostView* view, 
		const UINT rgba[4],
		UINT NumRects = 0U, 
		const D3D12_RECT* pRects = nullptr) = 0;

	virtual void ClearUnorderedAccessView(
		IFrostView* view, 
		const FLOAT rgba[4],
		UINT NumRects = 0U, 
		const D3D12_RECT* pRects = nullptr) = 0;

	virtual void DrawInstanced(
		UINT VertexCountPerInstance,
		UINT InstanceCount,
		UINT StartVertexLocation,
		UINT StartInstanceLocation) = 0;

	virtual void DrawIndexedInstanced(
		UINT IndexCountPerInstance,
		UINT InstanceCount,
		UINT StartIndexLocation,
		UINT BaseVertexLocation,
		UINT StartInstanceLocation) = 0;

	virtual void UpdateSubresources(
		IFrostResource* pFrostDestResource, 
		IFrostResource* pFrostIntermedia,
		UINT64 IntermediateOffset, 
		UINT FirstSubresource, 
		UINT NumSubResources, 
		D3D12_SUBRESOURCE_DATA* pSrcData) = 0;
};