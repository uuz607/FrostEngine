//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include"pch.h"
#include"FrameResource.h"
#include"SquidRoom.h"

FrameResource::FrameResource(IFrostPSO* pScenePso, IFrostPSO* pShadowMapPso, D3D12_VIEWPORT* pViewport,IFrostView* pNullView):
	m_pipelineState(pScenePso),
	m_pipelineStateShadowMap(pShadowMapPso),
	m_nullSrvView(pNullView)
{
	// Describe and create the shadow map texture.
	CD3DX12_RESOURCE_DESC shadowTexDesc(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		static_cast<UINT>(pViewport->Width), 
		static_cast<UINT>(pViewport->Height), 
		1,
		1,
		DXGI_FORMAT_R32_TYPELESS,
		1, 
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);


	D3D12_CLEAR_VALUE clearValue;		// Performance tip: Tell the runtime at resource creation the desired clear value.
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	g_Device->CreateDefaultHeapResource(
		&shadowTexDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		g_HeapAllocator,
		&m_shadowTexture);
	SetName(m_shadowTexture->GetD3D12Resource(), L"shadowTexture");
	// Get a handle to the start of the descriptor heap then offset 
	// it based on the frame resource index.

	// Describe and create the shadow depth view and cache the CPU 
	// descriptor handle.
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
	depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	g_Device->CreateDepthStencilView(
		&depthStencilViewDesc,
		g_DescriptorAllocator,
		m_shadowTexture,
		&m_shadowDepthView);

	// Get a handle to the start of the descriptor heap then offset it 
	// based on the existing textures and the frame resource index. Each 
	// frame has 1 SRV (shadow tex) and 2 CBVs.

	// Describe and create a shader resource view (SRV) for the shadow depth 
	// texture and cache the GPU descriptor handle. This SRV is for sampling 
	// the shadow map from our shader. It uses the same texture that we use 
	// as a depth-stencil during the shadow pass.
	D3D12_SHADER_RESOURCE_VIEW_DESC shadowSrvDesc = {};
	shadowSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	shadowSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	shadowSrvDesc.Texture2D.MipLevels = 1;
	shadowSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	g_Device->CreateShaderResourceView(
		&shadowSrvDesc,
		g_DescriptorAllocator,
		m_shadowTexture,
		&m_shadowTextureView);

	// Create the constant buffers.
	const UINT constantBufferSize = (sizeof(SceneConstantBuffer) + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1); // must be a multiple 256 bytes

	g_Device->CreateUploadHeapResource(
		&CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
		nullptr,
		g_HeapAllocator,
		&m_shadowConstantBuffer);

	g_Device->CreateUploadHeapResource(
		&CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
		nullptr,
		g_HeapAllocator,
		&m_sceneConstantBuffer);

	// Create the constant buffer views: one for the shadow pass and
	// another for the scene pass.
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.SizeInBytes = constantBufferSize;

	// Describe and create the shadow constant buffer view (CBV) and 
	// cache the GPU descriptor handle.
	cbvDesc.BufferLocation = m_shadowConstantBuffer->GetGPUVirtualAddress();

	g_Device->CreateConstantBufferView(
		&cbvDesc,
		g_DescriptorAllocator,
		m_shadowConstantBuffer,
		&m_shadowCbvView);

	// Describe and create the scene constant buffer view (CBV) and 
	// cache the GPU descriptor handle.
	cbvDesc.BufferLocation = m_sceneConstantBuffer->GetGPUVirtualAddress();

	g_Device->CreateConstantBufferView(
		&cbvDesc,
		g_DescriptorAllocator,
		m_sceneConstantBuffer,
		&m_sceneCbvView);

	CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
	ThrowIfFailed(m_shadowConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pShadowConstantBufferWO)));
	ThrowIfFailed(m_sceneConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pSceneConstantBufferWO)));
}

FrameResource::~FrameResource()
{
	m_shadowConstantBuffer = nullptr;
	m_sceneConstantBuffer = nullptr;

	m_shadowTexture = nullptr;

	m_shadowDepthView = nullptr;
	m_nullSrvView = nullptr;
	m_shadowTextureView = nullptr;
	m_shadowCbvView = nullptr;
	m_sceneCbvView = nullptr;
}

// Builds and writes constant buffers from scratch to the proper slots for 
// this frame resource.
void FrameResource::WriteConstantBuffers(D3D12_VIEWPORT* pViewport, Camera* pSceneCamera, Camera lightCams[NumLights], LightState lights[NumLights])
{
	SceneConstantBuffer sceneConsts = {}; 
	SceneConstantBuffer shadowConsts = {};
	
	// Scale down the world a bit.
	::XMStoreFloat4x4(&sceneConsts.model, XMMatrixScaling(0.1f, 0.1f, 0.1f));
	::XMStoreFloat4x4(&shadowConsts.model, XMMatrixScaling(0.1f, 0.1f, 0.1f));

	// The scene pass is drawn from the camera.
	pSceneCamera->Get3DViewProjMatrices(&sceneConsts.view, &sceneConsts.projection, 90.0f, pViewport->Width, pViewport->Height);

	// The light pass is drawn from the first light.
	lightCams[0].Get3DViewProjMatrices(&shadowConsts.view, &shadowConsts.projection, 90.0f, pViewport->Width, pViewport->Height);

	for (int i = 0; i < NumLights; i++)
	{
		memcpy(&sceneConsts.lights[i], &lights[i], sizeof(LightState));
		memcpy(&shadowConsts.lights[i], &lights[i], sizeof(LightState));
	}

	// The shadow pass won't sample the shadow map, but rather write to it.
	shadowConsts.sampleShadowMap = FALSE;

	// The scene pass samples the shadow map.
	sceneConsts.sampleShadowMap = TRUE;

	shadowConsts.ambientColor = sceneConsts.ambientColor = { 0.1f, 0.2f, 0.3f, 1.0f };

	memcpy(m_pSceneConstantBufferWO, &sceneConsts, sizeof(SceneConstantBuffer));
	memcpy(m_pShadowConstantBufferWO, &shadowConsts, sizeof(SceneConstantBuffer));
}

// Sets up the descriptor tables for the worker command list to use 
// resources provided by frame resource.
void FrameResource::Bind(IFrostCommandList* pCommandList, BOOL scenePass, IFrostView** pRtv, IFrostView* pDsv)
{
	if (scenePass)
	{
		// Scene pass. We use constant buf #2 and depth stencil #2
		// with rendering to the render target enabled.
		pCommandList->SetGraphicsDescriptorTable(2, m_shadowTextureView);		// Set the shadow texture as an SRV.
		pCommandList->SetGraphicsDescriptorTable(1, m_sceneCbvView);
		
		assert(pRtv != nullptr);
		assert(pDsv != nullptr);

		pCommandList->SetRenderTargets(1, pRtv, FALSE, pDsv);
	}
	else
	{
		// Shadow pass. We use constant buf #1 and depth stencil #1
		// with rendering to the render target disabled.
		pCommandList->SetGraphicsDescriptorTable(2, m_nullSrvView);// Set a null SRV for the shadow texture.
		pCommandList->SetGraphicsDescriptorTable(1, m_shadowCbvView);

		pCommandList->SetRenderTargets(0, nullptr, FALSE, m_shadowDepthView);	// No render target needed for the shadow pass.
	}
}
