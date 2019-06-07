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

#include "Camera.h"
#include "DXSampleHelper.h"
#include "D3D12Multithreading.h"

using namespace DirectX;
using namespace Microsoft::WRL;

class FrameResource 
{
public:

	FrameResource(IFrostPSO* pScenePso, IFrostPSO* pShadowMapPso, D3D12_VIEWPORT* pViewport,IFrostView* pNullView);
	~FrameResource();

	void Bind(IFrostCommandList* pCommandList, BOOL scenePass, IFrostView** pRtvHandle, IFrostView* pDsvHandle);
	void WriteConstantBuffers(D3D12_VIEWPORT* pViewport, Camera* pSceneCamera, Camera lightCams[NumLights], LightState lights[NumLights]);

	IFrostResource* GetShadowTexture()const
	{
		return m_shadowTexture;
	}

	IFrostView* GetShadowDepthView() const
	{
		return m_shadowDepthView;
	}
private:
	DX12_PTR(IFrostPSO)		 m_pipelineState;
	DX12_PTR(IFrostPSO)		 m_pipelineStateShadowMap;
	DX12_PTR(IFrostResource) m_shadowTexture;
	DX12_PTR(IFrostView)	 m_shadowDepthView;
	DX12_PTR(IFrostResource) m_shadowConstantBuffer;
	DX12_PTR(IFrostResource) m_sceneConstantBuffer;

	SceneConstantBuffer* m_pShadowConstantBufferWO;		// WRITE-ONLY pointer to the shadow pass constant buffer.
	SceneConstantBuffer* m_pSceneConstantBufferWO;		// WRITE-ONLY pointer to the scene pass constant buffer.

	DX12_PTR(IFrostView) m_nullSrvView;	// Null SRV for out of bounds behavior.
	DX12_PTR(IFrostView) m_shadowTextureView;
	DX12_PTR(IFrostView) m_shadowCbvView;
	DX12_PTR(IFrostView) m_sceneCbvView;

};
