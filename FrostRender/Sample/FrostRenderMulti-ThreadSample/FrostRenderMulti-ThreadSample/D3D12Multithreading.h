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

#pragma once

#include "pch.h"
#include "DXSample.h"
#include "Camera.h"
#include "StepTimer.h"
#include "SquidRoom.h"

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().

class FrameResource;

struct LightState
{
	XMFLOAT4 position;
	XMFLOAT4 direction;
	XMFLOAT4 color;
	XMFLOAT4 falloff;

	XMFLOAT4X4 view;
	XMFLOAT4X4 projection;
};

struct SceneConstantBuffer
{
	XMFLOAT4X4 model;
	XMFLOAT4X4 view;
	XMFLOAT4X4 projection;
	XMFLOAT4 ambientColor;
	BOOL sampleShadowMap;
	BOOL padding[3];		// Must be aligned to be made up of N float4s.
	LightState lights[NumLights];
};

extern ISystem* g_ISystem;
extern IThreadManager* g_IThreadManager;
extern ILog* g_ILog;

extern DX12_PTR(IFrostSwapChain)    g_SwapChain;
extern DX12_PTR(IFrostDevice)		g_Device;

extern DX12_PTR(IFrostFactory) g_Factory;
extern DX12_PTR(IFrostAdapter) g_Adapter;


extern DX12_PTR(IFrostDescriptorAllocator) g_DescriptorAllocator;
extern DX12_PTR(IFrostHeapAllocator)	   g_HeapAllocator;

class D3D12Multithreading : public DXSample
{
public:
	D3D12Multithreading(UINT width, UINT height, std::wstring name);
	virtual ~D3D12Multithreading();

	static D3D12Multithreading* Get() { return s_app; }

	virtual void OnInit();
	virtual void OnUpdate();
	virtual void OnRender();
	virtual void OnDestroy();
	virtual void OnKeyDown(UINT8 key);
	virtual void OnKeyUp(UINT8 key);

private:
	struct InputState
	{
		bool rightArrowPressed;
		bool leftArrowPressed;
		bool upArrowPressed;
		bool downArrowPressed;
		bool animate;
	};
	
	// Pipeline objects.
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	DX12_PTR(IFrostCommandList) m_CopyCommandList;
	DX12_PTR(IFrostCommandList) m_CommandLists[CommandListCount];
	DX12_PTR(IFrostCommandList) m_ShadowCommandLists[NumContexts];
	DX12_PTR(IFrostCommandList) m_SceneCommandLists[NumContexts];

	DX12_PTR(IFrostPSO)	 m_PipelineStateShadowMap;
	DX12_PTR(IFrostPSO)	 m_PipelineState;
	DX12_PTR(IFrostRootSignature) m_RootSignature;

	DX12_PTR(IFrostResource) m_RenderTargets[FrameCount];
	DX12_PTR(IFrostView)	 m_RenderTargetViews[FrameCount];

	DX12_PTR(IFrostResource) m_DepthStencil;
	DX12_PTR(IFrostView)	 m_DepthStencilView;

	// App resources.
	DX12_PTR(IFrostSamplerState) m_WrapSampler;
	DX12_PTR(IFrostSamplerState) m_ClampSampler;

	DX12_PTR(IFrostView) m_DiffuseTextureHandle;
	DX12_PTR(IFrostView) m_NormalTextureHandle;

	DX12_PTR(IFrostResource) m_Textures[_countof(SampleAssets::Textures)];
	DX12_PTR(IFrostResource) m_TextureUploads[_countof(SampleAssets::Textures)];
	DX12_PTR(IFrostView)	 m_TextureViews[_countof(SampleAssets::Textures)];

	DX12_PTR(IFrostResource) m_ShadowTexture;
	DX12_PTR(IFrostView)	 m_ShadowTextureHandle;

	DX12_PTR(IFrostResource) m_ShadowConstantBuffer;
	DX12_PTR(IFrostView)	 m_ShadowConstantBufferView;

	DX12_PTR(IFrostResource) m_SceneConstantBuffer;
	DX12_PTR(IFrostView)	 m_SceneConstantBufferView;

	DX12_PTR(IFrostResource) m_indexBuffer;
	DX12_PTR(IFrostResource) m_indexBufferUpload;
	DX12_PTR(IFrostView)	 m_indexBufferView;

	DX12_PTR(IFrostResource) m_vertexBuffer;
	DX12_PTR(IFrostResource) m_vertexBufferUpload;
	DX12_PTR(IFrostView)	 m_vertexBufferView;

	DX12_PTR(IFrostCommandList) m_commandLists[CommandListCount];

	DX12_PTR(IFrostCommandList) m_shadowCommandLists[NumContexts];

	DX12_PTR(IFrostCommandList) m_sceneCommandLists[NumContexts];

	SceneConstantBuffer* m_pShadowConstantBufferWO;		// WRITE-ONLY pointer to the shadow pass constant buffer.
	SceneConstantBuffer* m_pSceneConstantBufferWO;		// WRITE-ONLY pointer to the scene pass constant buffer.


	UINT m_rtvDescriptorSize;
	InputState m_keyboardInput;
	LightState m_lights[NumLights];
	Camera m_lightCameras[NumLights];
	Camera m_camera;
	StepTimer m_timer;
	StepTimer m_cpuTimer;
	int m_titleCount;
	double m_cpuTime;

	// Synchronization objects.
	HANDLE m_workerBeginShadowFrame[NumContexts];
	HANDLE m_workerFinishShadowPass[NumContexts];
	HANDLE m_workerFinishedRenderFrame[NumContexts];
	HANDLE m_EndMidFrame[NumContexts];
	HANDLE m_threadHandles[NumContexts];
	UINT m_frameIndex;

	
	// Singleton object so that worker threads can share members.
	static D3D12Multithreading* s_app; 

	// Frame resources.
	FrameResource* m_frameResources[FrameCount];
	FrameResource* m_pCurrentFrameResource;
	int m_currentFrameResourceIndex;

	struct ThreadParameter
	{
		int threadIndex;
	};
	ThreadParameter m_threadParameters[NumContexts];

	void ShadowThread(int threadIndex);
	void SetCommonPipelineState(IFrostCommandList* pCommandList);

	void LoadPipeline();
	void LoadAssets();
	void LoadContexts();
	void BeginFrame();
	void MidFrame();
	void EndFrame();
};
