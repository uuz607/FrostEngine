#pragma once

#include"Common\3rdParty\fasthash.h"
#include"Common\3rdParty\fasthash.inl"


#if FROST_PLATFORM_64BIT && FROST_PLATFORM_DESKTOP
	#define DX12_LINKEDADAPTER            true
	#define DX12_LINKEDADAPTER_SIMULATION 0
#endif

#define DX12_NEW_RAW(P) FrostDX12::PassAddRef(new P)

#define INVALID_CPU_DESCRIPTOR_HANDLE     CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT)
#define INVALID_GPU_DESCRIPTOR_HANDLE     CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT)
#define INVALID_CPU_GPU_DESCRIPTOR_HANDLE FrostDX12::DESCRIPTOR_HANDLE()

#define ALIGN(P,A)	(((P)+(A-1))&~(A-1))

namespace FrostDX12
{
	static const UINT CONSTANT_BUFFER_ELEMENT_SIZE = 16U;

	template<typename T>
	static T* PassAddRef(T* ptr)
	{
		if (ptr)
		{
			ptr->AddRef();
		}

		return ptr;
	}

	template<typename T>
	static T* PassAddRef(const _smart_ptr<T>& ptr)
	{
		if (ptr)
		{
			ptr.get()->AddRef();
		}

		return ptr.get();
	}

	typedef uint32_t Hash;
	template<size_t length>
	inline Hash ComputeSmallHash(const void* data, UINT seed = 666)
	{
		return fasthash::fasthash32<length>(data, seed);
	}

	typedef 
	struct NodeMaskCV
	{
		UINT creationMask;
		UINT visibilityMask;
	}NODE64;

	struct DESCRIPTOR_HANDLE
	{
		D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;

		DESCRIPTOR_HANDLE()
		{
			CPUHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
			GPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
		}
	};

	inline bool IsDXGIFormatCompressed(DXGI_FORMAT format)
	{
		return (format >= DXGI_FORMAT_BC1_TYPELESS && format <= DXGI_FORMAT_BC5_SNORM) ||
			   (format >= DXGI_FORMAT_BC6H_TYPELESS && format <= DXGI_FORMAT_BC7_UNORM_SRGB);
	}
}

