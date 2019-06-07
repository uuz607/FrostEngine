#include"pch.h"
#include"FrostRenderDXGI.h"
#include"DX12GIFactory.h"
#include"DX12Device.h"
#include"DX12GIAdapter.h"

using namespace FrostDX12;

void CreateFrostFactory(IFrostFactory** ppFactory)
{
	(*ppFactory) = CDXGIFactory::Create();
}

void CreateFrostDevice(IFrostAdapter* pFrostAdapter, IFrostDevice** ppDevice)
{
	(*ppDevice) = CDevice::Create(pFrostAdapter);
}


