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
#include "D3D12Multithreading.h"
#include "FrameResource.h"

D3D12Multithreading* D3D12Multithreading::s_app = nullptr;

ISystem* g_ISystem;
IThreadManager* g_IThreadManager;
ILog*	g_ILog;

DX12_PTR(IFrostSwapChain) g_SwapChain;
DX12_PTR(IFrostDevice)	  g_Device;

DX12_PTR(IFrostFactory) g_Factory;
DX12_PTR(IFrostAdapter) g_Adapter;

DX12_PTR(IFrostDescriptorAllocator) g_DescriptorAllocator;
DX12_PTR(IFrostHeapAllocator)	   g_HeapAllocator;

D3D12Multithreading::D3D12Multithreading(UINT width, UINT height, std::wstring name) :
	DXSample(width, height, name),
	m_frameIndex(0),
	m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
	m_keyboardInput(),
	m_titleCount(0),
	m_cpuTime(0),
	m_rtvDescriptorSize(0),
	m_currentFrameResourceIndex(0),
	m_pCurrentFrameResource(nullptr)
{
	s_app = this;

	m_keyboardInput.animate = true;
}

D3D12Multithreading::~D3D12Multithreading()
{
	s_app = nullptr;
}

void D3D12Multithreading::OnInit()
{
	g_ISystem = CreateSystem();
	g_ILog = CreateLog();
	g_IThreadManager = CreateThreadManager();
	g_ISystem->SetIThreadManager(g_IThreadManager);
	g_ISystem->SetILog(g_ILog);

	LoadPipeline();
	LoadAssets();
	LoadContexts();
}

// Load the rendering pipeline dependencies.
void D3D12Multithreading::LoadPipeline()
{
	CreateFrostFactory(&g_Factory);
	
	g_Factory->EnumAdapter(1U, &g_Adapter);

	CreateFrostDevice(g_Adapter, &g_Device);

	DESCRIPTOR_ALLOCATOR_DESC descAllocaDesc =
	{
		{ D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 256, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1U },
		{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 256, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1U },
		{ D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 256, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1U },
		{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1U }
	};

	g_Device->CreateDescriptorAllocator(descAllocaDesc, &g_DescriptorAllocator);

	HEAP_ALLOCATOR_DESC heapAllocaDesc =
	{
		CD3DX12_HEAP_DESC(1024 * 1024 * 100,D3D12_HEAP_TYPE_DEFAULT),
		CD3DX12_HEAP_DESC(1024 * 1024 * 100,D3D12_HEAP_TYPE_UPLOAD),
		CD3DX12_HEAP_DESC(1024 * 1024 * 10,D3D12_HEAP_TYPE_READBACK),
	};

	g_Device->CreateHeapAllocator(heapAllocaDesc, &g_HeapAllocator);

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	g_Factory->CreateSwapChain(
		g_Device, 
		Win32Application::GetHwnd(), 
		&swapChainDesc, 
		nullptr, 
		nullptr, 
		&g_SwapChain);
}

// Load the sample assets.
void D3D12Multithreading::LoadAssets()
{
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[4]; // Perfomance TIP: Order from most frequent to least frequent.
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);	// 2 frequently changed diffuse + normal textures - using registers t1 and t2.
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);	// 1 frequently changed constant buffer.
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);												// 1 infrequently changed shadow texture - starting in register t0.
		ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);											// 2 static samplers.

		CD3DX12_ROOT_PARAMETER1 rootParameters[4];
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		g_Device->CreateRootSignature(&rootSignatureDesc, true, m_RootSignature);
	}

	{
		ID3DBlob* pIVertexBlob;
		ID3DBlob* pIPixelBlob;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

		::D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &pIVertexBlob, nullptr);
		::D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pIPixelBlob, nullptr);

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
		inputLayoutDesc.pInputElementDescs = SampleAssets::StandardVertexDescription;
		inputLayoutDesc.NumElements = _countof(SampleAssets::StandardVertexDescription);

		CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc;
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		depthStencilDesc.StencilEnable = FALSE;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = inputLayoutDesc;
		psoDesc.pRootSignature = m_RootSignature->GetD3D12RootSignature();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(pIVertexBlob);
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pIPixelBlob);
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = depthStencilDesc;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.NodeMask = 1U;

		g_Device->CreateGraphicsPipelineState(psoDesc, m_PipelineState);

		psoDesc.PS = CD3DX12_SHADER_BYTECODE(0, 0);
		psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
		psoDesc.NumRenderTargets = 0;

		g_Device->CreateGraphicsPipelineState(psoDesc, m_PipelineStateShadowMap);

	}
	
	g_Device->CreateCommandList(CMDQUEUE_COPY, m_CopyCommandList);

	// Create render target views (RTVs).
	for (UINT i = 0; i < FrameCount; i++)
	{
		g_SwapChain->GetBuffer(i, &m_RenderTargets[i]);

		SetNameIndexed(m_RenderTargets[i]->GetD3D12Resource(), L"RenderTargets", i);

		g_Device->CreateRenderTargetView(nullptr, g_DescriptorAllocator, m_RenderTargets[i], &m_RenderTargetViews[i]);
	}

	// Create the depth stencil.
	{
		CD3DX12_RESOURCE_DESC shadowTextureDesc(
			D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			0,
			static_cast<UINT>(m_viewport.Width), 
			static_cast<UINT>(m_viewport.Height), 
			1,
			1,
			DXGI_FORMAT_D32_FLOAT,
			1, 
			0,
			D3D12_TEXTURE_LAYOUT_UNKNOWN,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);

		D3D12_CLEAR_VALUE clearValue;	// Performance tip: Tell the runtime at resource creation the desired clear value.
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		g_Device->CreateDefaultHeapResource(
			&shadowTextureDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			g_HeapAllocator,
			&m_DepthStencil);

		// Create the depth stencil view.
		g_Device->CreateDepthStencilView(nullptr, g_DescriptorAllocator, m_DepthStencil, &m_DepthStencilView);
	}

	// Load scene assets.
	UINT fileSize = 0;
	UINT8* pAssetData;
	ThrowIfFailed(ReadDataFromFile(L"SquidRoom.bin", &pAssetData, &fileSize));

	// Create the vertex buffer.
	{
		g_Device->CreateDefaultHeapResource(
			&CD3DX12_RESOURCE_DESC::Buffer(SampleAssets::VertexDataSize),
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			g_HeapAllocator,
			&m_vertexBuffer);

		{
			g_Device->CreateUploadHeapResource(
				&CD3DX12_RESOURCE_DESC::Buffer(SampleAssets::VertexDataSize),
				nullptr,
				g_HeapAllocator,
				&m_vertexBufferUpload);

			// Copy data to the upload heap and then schedule a copy 
			// from the upload heap to the vertex buffer.
			D3D12_SUBRESOURCE_DATA vertexData = {};
			vertexData.pData = pAssetData + SampleAssets::VertexDataOffset;
			vertexData.RowPitch = SampleAssets::VertexDataSize;
			vertexData.SlicePitch = vertexData.RowPitch;

			m_CopyCommandList->UpdateSubresources(m_vertexBuffer, m_vertexBufferUpload, 0, 0, 1, &vertexData);
		}

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView.SizeInBytes = SampleAssets::VertexDataSize;
		vertexBufferView.StrideInBytes = SampleAssets::StandardVertexStride;

		g_Device->CreateVertexBufferView(vertexBufferView, m_vertexBuffer, &m_vertexBufferView);
	}

	// Create the index buffer.
	{
		{
			g_Device->CreateDefaultHeapResource(
				&CD3DX12_RESOURCE_DESC::Buffer(SampleAssets::IndexDataSize),
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				g_HeapAllocator,
				&m_indexBuffer);

			g_Device->CreateUploadHeapResource(
				&CD3DX12_RESOURCE_DESC::Buffer(SampleAssets::IndexDataSize),
				nullptr,
				g_HeapAllocator,
				&m_indexBufferUpload);

			// Copy data to the upload heap and then schedule a copy 
			// from the upload heap to the index buffer.
			D3D12_SUBRESOURCE_DATA indexData = {};
			indexData.pData = pAssetData + SampleAssets::IndexDataOffset;
			indexData.RowPitch = SampleAssets::IndexDataSize;
			indexData.SlicePitch = indexData.RowPitch;
			
			m_CopyCommandList->UpdateSubresources(m_indexBuffer, m_indexBufferUpload, 0, 0, 1, &indexData);
		}

		D3D12_INDEX_BUFFER_VIEW indexBufferView;
		indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
		indexBufferView.SizeInBytes = SampleAssets::IndexDataSize;
		indexBufferView.Format = SampleAssets::StandardIndexFormat;

		g_Device->CreateIndexBufferView(indexBufferView, m_indexBuffer, &m_indexBufferView);
	}

	// Create shader resources.
	{
		{
			// Describe and create 2 null SRVs. Null descriptors are needed in order 
			// to achieve the effect of an "unbound" resource.
			D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = {};
			nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			nullSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			nullSrvDesc.Texture2D.MipLevels = 1;
			nullSrvDesc.Texture2D.MostDetailedMip = 0;
			nullSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

			g_Device->CreateShaderResourceView(&nullSrvDesc, g_DescriptorAllocator, nullptr, &m_DiffuseTextureHandle);

			g_Device->CreateShaderResourceView(&nullSrvDesc, g_DescriptorAllocator, nullptr, &m_NormalTextureHandle);
		}

		// Create each texture and SRV descriptor.
		const UINT srvCount = _countof(SampleAssets::Textures);

		for (UINT i = 0; i < srvCount; ++i)
		{
			// Describe and create a Texture2D.
			const SampleAssets::TextureResource &tex = SampleAssets::Textures[i];
			CD3DX12_RESOURCE_DESC texDesc(
				D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				0,
				tex.Width, 
				tex.Height, 
				1,
				static_cast<UINT16>(tex.MipLevels),
				tex.Format,
				1, 
				0,
				D3D12_TEXTURE_LAYOUT_UNKNOWN,
				D3D12_RESOURCE_FLAG_NONE);

			g_Device->CreateDefaultHeapResource(
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				g_HeapAllocator,
				&m_Textures[i]);

			SetNameIndexed(m_Textures[i]->GetD3D12Resource(), L"Textures", i);

			const UINT subresourceCount = texDesc.DepthOrArraySize * texDesc.MipLevels;
			UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_Textures[i]->GetD3D12Resource(), 0, subresourceCount);

			g_Device->CreateUploadHeapResource(
				&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
				nullptr,
				g_HeapAllocator,
				&m_TextureUploads[i]);

			{
				// Copy data to the intermediate upload heap and then schedule a copy
				// from the upload heap to the Texture2D.
				D3D12_SUBRESOURCE_DATA textureData = {};
				textureData.pData = pAssetData + tex.Data->Offset;
				textureData.RowPitch = tex.Data->Pitch;
				textureData.SlicePitch = tex.Data->Size;

				m_CopyCommandList->UpdateSubresources(m_Textures[i], m_TextureUploads[i], 0, 0, subresourceCount, &textureData);
			}

			// Describe and create an SRV.
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = tex.Format;
			srvDesc.Texture2D.MipLevels = tex.MipLevels;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

			g_Device->CreateShaderResourceView(
				&srvDesc,
				g_DescriptorAllocator,
				m_Textures[i],
				&m_TextureViews[i]);
		}
	}
	 
	free(pAssetData);

	g_Device->HoldBack(CMDQUEUE_GRAPHICS, m_CopyCommandList);

	g_Device->ExecuteCommandList(m_CopyCommandList);

	// Create the samplers.
	{
		// Describe and create the wrapping sampler, which is used for 
		// sampling diffuse/normal maps.
		D3D12_SAMPLER_DESC wrapSamplerDesc = {};
		wrapSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		wrapSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		wrapSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		wrapSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		wrapSamplerDesc.MinLOD = 0;
		wrapSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		wrapSamplerDesc.MipLODBias = 0.0f;
		wrapSamplerDesc.MaxAnisotropy = 1;
		wrapSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		wrapSamplerDesc.BorderColor[0] = wrapSamplerDesc.BorderColor[1] = wrapSamplerDesc.BorderColor[2] = wrapSamplerDesc.BorderColor[3] = 0;
		
		g_Device->CreateSampler(
			&wrapSamplerDesc,
			g_DescriptorAllocator,
			&m_WrapSampler);

		// Describe and create the point clamping sampler, which is 
		// used for the shadow map.
		D3D12_SAMPLER_DESC clampSamplerDesc = {};
		clampSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		clampSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		clampSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		clampSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		clampSamplerDesc.MipLODBias = 0.0f;
		clampSamplerDesc.MaxAnisotropy = 1;
		clampSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		clampSamplerDesc.BorderColor[0] = clampSamplerDesc.BorderColor[1] = clampSamplerDesc.BorderColor[2] = clampSamplerDesc.BorderColor[3] = 0;
		clampSamplerDesc.MinLOD = 0;
		clampSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		
		g_Device->CreateSampler(&clampSamplerDesc, g_DescriptorAllocator, &m_ClampSampler);
	}

	// Create lights.
	for (int i = 0; i < NumLights; i++)
	{
		// Set up each of the light positions and directions (they all start 
		// in the same place).
		m_lights[i].position = { 0.0f, 15.0f, -30.0f, 1.0f };
		m_lights[i].direction = { 0.0, 0.0f, 1.0f, 0.0f };
		m_lights[i].falloff = { 800.0f, 1.0f, 0.0f, 1.0f };
		m_lights[i].color = { 0.7f, 0.7f, 0.7f, 1.0f };

		XMVECTOR eye = XMLoadFloat4(&m_lights[i].position);
		XMVECTOR at = XMVectorAdd(eye, XMLoadFloat4(&m_lights[i].direction));
		XMVECTOR up = { 0, 1, 0 };

		m_lightCameras[i].Set(eye, at, up);
	}
	
	// Create frame resources.
	for (int i = 0; i < FrameCount; i++)
	{
		m_frameResources[i] = new FrameResource(m_PipelineState, m_PipelineStateShadowMap, &m_viewport, m_DiffuseTextureHandle);
		m_frameResources[i]->WriteConstantBuffers(&m_viewport, &m_camera, m_lightCameras, m_lights);
	}
	m_currentFrameResourceIndex = 0;
	m_pCurrentFrameResource = m_frameResources[m_currentFrameResourceIndex];

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	

	// Wait for the command list to execute; we are reusing the same command 
	// list in our main loop but for now, we just want to wait for setup to 
	// complete before continuing.
}


// Initialize threads and events.
void D3D12Multithreading::LoadContexts()
{
#if !SINGLETHREADED
	struct threadwrapper
	{
		static unsigned int WINAPI thunk(LPVOID lpParameter)
		{
			ThreadParameter* parameter = reinterpret_cast<ThreadParameter*>(lpParameter);
			D3D12Multithreading::Get()->ShadowThread(parameter->threadIndex);
			return 0;
		}
	};

	for (int i = 0; i < NumContexts; i++)
	{
		m_workerBeginShadowFrame[i] = CreateEvent(
			NULL,
			FALSE,
			FALSE,
			NULL);

		m_workerFinishedRenderFrame[i] = CreateEvent(
			NULL,
			FALSE,
			FALSE,
			NULL);

		m_workerFinishShadowPass[i] = CreateEvent(
			NULL,
			FALSE,
			FALSE,
			NULL);
		
		m_EndMidFrame[i] = CreateEvent(
			NULL,
			FALSE,
			FALSE,
			NULL);

		m_threadParameters[i].threadIndex = i;

		m_threadHandles[i] = reinterpret_cast<HANDLE>(_beginthreadex(
			nullptr,
			0,
			threadwrapper::thunk,
			reinterpret_cast<LPVOID>(&m_threadParameters[i]),
			0,
			nullptr));


		assert(m_workerBeginShadowFrame[i] != NULL);
		assert(m_workerFinishedRenderFrame[i] != NULL);
		assert(m_threadHandles[i] != NULL);
		assert(m_EndMidFrame[i] != NULL);
	}
#endif
}

// Update frame-based values.
void D3D12Multithreading::OnUpdate()
{
	m_timer.Tick(NULL);

	// Get current GPU progress against submitted workload. Resources still scheduled 
	// for GPU execution cannot be modified or else undefined behavior will result.

	// Move to the next frame resource.
	m_currentFrameResourceIndex = (m_currentFrameResourceIndex + 1) % FrameCount;
	m_pCurrentFrameResource = m_frameResources[m_currentFrameResourceIndex];

	m_cpuTimer.Tick(NULL);
	float frameTime = static_cast<float>(m_timer.GetElapsedSeconds());
	float frameChange = 2.0f * frameTime;

	if (m_keyboardInput.leftArrowPressed)
		m_camera.RotateYaw(-frameChange);
	if (m_keyboardInput.rightArrowPressed)
		m_camera.RotateYaw(frameChange);
	if (m_keyboardInput.upArrowPressed)
		m_camera.RotatePitch(frameChange);
	if (m_keyboardInput.downArrowPressed)
		m_camera.RotatePitch(-frameChange);

	if (m_keyboardInput.animate)
	{
		for (int i = 0; i < NumLights; i++)
		{
			float direction = frameChange * pow(-1.0f, i);
			XMStoreFloat4(&m_lights[i].position, XMVector4Transform(XMLoadFloat4(&m_lights[i].position), XMMatrixRotationY(direction)));

			XMVECTOR eye = XMLoadFloat4(&m_lights[i].position);
			XMVECTOR at = { 0.0f, 8.0f, 0.0f };
			XMStoreFloat4(&m_lights[i].direction, XMVector3Normalize(XMVectorSubtract(at, eye)));
			XMVECTOR up = { 0.0f, 1.0f, 0.0f };
			m_lightCameras[i].Set(eye, at, up);

			m_lightCameras[i].Get3DViewProjMatrices(&m_lights[i].view, &m_lights[i].projection, 90.0f, static_cast<float>(m_width), static_cast<float>(m_height));
		}
	}

	m_pCurrentFrameResource->WriteConstantBuffers(&m_viewport, &m_camera, m_lightCameras, m_lights);
}

// Render the scene.
void D3D12Multithreading::OnRender()
{
	BeginFrame();
	
#if SINGLETHREADED
	for (int i = 0; i < NumContexts; i++)
	{
		WorkerThread(i);
	}
	MidFrame();
	EndFrame();
	m_commandQueue->ExecuteCommandLists(_countof(m_pCurrentFrameResource->m_batchSubmit), m_pCurrentFrameResource->m_batchSubmit);
#else
	for (int i = 0; i < NumContexts; i++)
	{
		SetEvent(m_workerBeginShadowFrame[i]); // Tell each worker to start drawing.
	}

	WaitForMultipleObjects(NumContexts, m_workerFinishShadowPass, TRUE, INFINITE);

	MidFrame();
	// You can execute command lists on any thread. Depending on the work 
	// load, apps can choose between using ExecuteCommandLists on one thread 
	// vs ExecuteCommandList from multiple threads.
	for (int i = 0; i < NumContexts; i++)
	{
		SetEvent(m_EndMidFrame[i]); // Tell each worker to start drawing.
	}

	WaitForMultipleObjects(NumContexts, m_workerFinishedRenderFrame, TRUE, INFINITE);

	EndFrame();
	// Submit remaining command lists.
#endif

	m_cpuTimer.Tick(NULL);
	if (m_titleCount == TitleThrottle)
	{
		WCHAR cpu[64];
		swprintf_s(cpu, L"%.4f CPU", m_cpuTime / m_titleCount);
		SetCustomWindowText(cpu);

		m_titleCount = 0;
		m_cpuTime = 0;
	}
	else
	{
		m_titleCount++;
		m_cpuTime += m_cpuTimer.GetElapsedSeconds() * 1000;
		m_cpuTimer.ResetElapsedTime();
	}
}

void D3D12Multithreading::OnDestroy()
{

	// Close thread events and thread handles.
	for (int i = 0; i < NumContexts; i++)
	{
		CloseHandle(m_workerBeginShadowFrame[i]);
		CloseHandle(m_workerFinishShadowPass[i]);
		CloseHandle(m_workerFinishedRenderFrame[i]);
		CloseHandle(m_EndMidFrame[i]);
	}

	for (int i = 0; i < _countof(m_frameResources); i++)
	{
		delete m_frameResources[i];
	}
}

void D3D12Multithreading::OnKeyDown(UINT8 key)
{
	switch (key)
	{
	case VK_LEFT:
		m_keyboardInput.leftArrowPressed = true;
		break;
	case VK_RIGHT:
		m_keyboardInput.rightArrowPressed = true;
		break;
	case VK_UP:
		m_keyboardInput.upArrowPressed = true;
		break;
	case VK_DOWN:
		m_keyboardInput.downArrowPressed = true;
		break;
	case VK_SPACE:
		m_keyboardInput.animate = !m_keyboardInput.animate;
		break;
	}
}

void D3D12Multithreading::OnKeyUp(UINT8 key)
{
	switch (key)
	{
	case VK_LEFT:
		m_keyboardInput.leftArrowPressed = false;
		break;
	case VK_RIGHT:
		m_keyboardInput.rightArrowPressed = false;
		break;
	case VK_UP:
		m_keyboardInput.upArrowPressed = false;
		break;
	case VK_DOWN:
		m_keyboardInput.downArrowPressed = false;
		break;
	}
}

// Assemble the CommandListPre command list.
void D3D12Multithreading::BeginFrame()
{
	g_Device->CreateCommandList(CMDQUEUE_GRAPHICS, m_CommandLists[CommandListPre]);
	m_CommandLists[CommandListPre]->SetPipelineState(m_PipelineState);

	// Indicate that the back buffer will be used as a render target.
	m_CommandLists[CommandListPre]->ResourceBarrier(m_RenderTargets[m_frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET);


	// Clear the render target and depth stencil.
	const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_CommandLists[CommandListPre]->ClearRenderTargetView(m_RenderTargetViews[m_frameIndex], clearColor, 0, nullptr);
	m_CommandLists[CommandListPre]->ClearDepthStencilView(m_DepthStencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	m_CommandLists[CommandListPre]->ClearDepthStencilView(
		m_pCurrentFrameResource->GetShadowDepthView(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	g_Device->ExecuteCommandList(m_CommandLists[CommandListPre]);
}

// Assemble the CommandListMid command list.
void D3D12Multithreading::MidFrame()
{
	// Transition our shadow map from the shadow pass to readable in the scene pass.
	g_Device->CreateCommandList(CMDQUEUE_GRAPHICS, m_CommandLists[CommandListMid]);
	m_CommandLists[CommandListMid]->SetPipelineState(m_PipelineState);

	m_CommandLists[CommandListMid]->ResourceBarrier(
		m_pCurrentFrameResource->GetShadowTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	g_Device->ExecuteCommandList(m_CommandLists[CommandListMid]);
}

// Assemble the CommandListPost command list.
void D3D12Multithreading::EndFrame()
{
	g_Device->CreateCommandList(CMDQUEUE_GRAPHICS, m_CommandLists[CommandListPost]);
	m_CommandLists[CommandListPost]->SetPipelineState(m_PipelineState);

	m_CommandLists[CommandListPost]->ResourceBarrier(
		m_pCurrentFrameResource->GetShadowTexture(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

	// Indicate that the back buffer will now be used to present.
	m_CommandLists[CommandListPost]->ResourceBarrier(m_RenderTargets[m_frameIndex], D3D12_RESOURCE_STATE_PRESENT);
	
	g_Device->ExecuteCommandList(m_CommandLists[CommandListPost]);

	// Present and update the frame index for the next frame.
	g_SwapChain->Present(1, 0);

	m_frameIndex = g_SwapChain->GetCurrentBackBufferIndex();
}


// Worker thread body. workerIndex is an integer from 0 to NumContexts 
// describing the worker's thread index.
void D3D12Multithreading::ShadowThread(int threadIndex)
{
	assert(threadIndex >= 0);
	assert(threadIndex < NumContexts);
#if !SINGLETHREADED

	while (threadIndex >= 0 && threadIndex < NumContexts)
	{
		WaitForSingleObject(m_workerBeginShadowFrame[threadIndex], INFINITE);
#endif
		g_Device->CreateCommandList(CMDQUEUE_GRAPHICS, m_ShadowCommandLists[threadIndex]);

		m_ShadowCommandLists[threadIndex]->SetPipelineState(m_PipelineStateShadowMap);
		
		// Populate the command list.
		SetCommonPipelineState(m_ShadowCommandLists[threadIndex]);
		m_pCurrentFrameResource->Bind(m_ShadowCommandLists[threadIndex], FALSE, nullptr, nullptr);	// No need to pass RTV or DSV descriptor heap.

		// Set null SRVs for the diffuse/normal textures.
		m_ShadowCommandLists[threadIndex]->SetGraphicsDescriptorTable(0, m_DiffuseTextureHandle);

		// Distribute objects over threads by drawing only 1/NumContexts 
		// objects per worker (i.e. every object such that objectnum % 
		// NumContexts == threadIndex).

		for (int j = threadIndex; j < _countof(SampleAssets::Draws); j += NumContexts)
		{
			SampleAssets::DrawParameters drawArgs = SampleAssets::Draws[j];

			m_ShadowCommandLists[threadIndex]->DrawIndexedInstanced(drawArgs.IndexCount, 1, drawArgs.IndexStart, drawArgs.VertexBase, 0);
		}

		g_Device->ExecuteCommandList(m_ShadowCommandLists[threadIndex]);

#if !SINGLETHREADED
		// Submit shadow pass.
		SetEvent(m_workerFinishShadowPass[threadIndex]);
#endif
		WaitForSingleObject(m_EndMidFrame[threadIndex], INFINITE);

		g_Device->CreateCommandList(CMDQUEUE_GRAPHICS, m_SceneCommandLists[threadIndex]);

		m_SceneCommandLists[threadIndex]->SetPipelineState(m_PipelineState);

		SetCommonPipelineState(m_SceneCommandLists[threadIndex]);
		m_pCurrentFrameResource->Bind(m_SceneCommandLists[threadIndex], TRUE, &m_RenderTargetViews[m_frameIndex], m_DepthStencilView);

		for (int j = threadIndex; j < _countof(SampleAssets::Draws); j += NumContexts)
		{
			SampleAssets::DrawParameters drawArgs = SampleAssets::Draws[j];
			m_SceneCommandLists[threadIndex]->SetGraphicsDescriptorTable(0, m_TextureViews[drawArgs.DiffuseTextureIndex]);

			m_SceneCommandLists[threadIndex]->DrawIndexedInstanced(drawArgs.IndexCount, 1, drawArgs.IndexStart, drawArgs.VertexBase, 0);
		}

		g_Device->ExecuteCommandList(m_SceneCommandLists[threadIndex]);

#if !SINGLETHREADED
		// Tell main thread that we are done.
		SetEvent(m_workerFinishedRenderFrame[threadIndex]);
#endif
	}
}

void D3D12Multithreading::SetCommonPipelineState(IFrostCommandList* pCommandList)
{
	pCommandList->SetGraphicsRootSignature(m_RootSignature);

	pCommandList->SetDescriptorHeaps(g_DescriptorAllocator);

	pCommandList->SetViewports(1, &m_viewport);
	pCommandList->SetScissorRects(1, &m_scissorRect);
	pCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->SetVertexBuffers(0, 1, &m_vertexBufferView);
	pCommandList->SetIndexBuffer(m_indexBufferView);
	pCommandList->SetGraphicsDescriptorTable(3, m_WrapSampler);
	pCommandList->SetStencilRef(0);

	// Render targets and depth stencil are set elsewhere because the 
	// depth stencil depends on the frame resource being used.

	// Constant buffers are set elsewhere because they depend on the 
	// frame resource being used.

	// SRVs are set elsewhere because they change based on the object 
	// being drawn.
}

