#include"pch.h"
#include"DX12SamplerState.h"

namespace FrostDX12
{
	/*--------------Interface implemtation--------------*/
	void CSamplerState::AddRef()
	{
		::_InterlockedIncrement(&m_RefCount);
	}

	void CSamplerState::Release()
	{
		LONG refCount = ::_InterlockedDecrement(&m_RefCount);
		if (!refCount)
		{
			delete this;
		}
	}

	void CSamplerState::Invalidate()
	{
		m_pDescAlloca->InvalidateSampler(&m_SamplerDesc);
	}

	/*--------------CSamplerState implemtation--------------*/
	CSamplerState::CSamplerState(CDescriptorAllocator* pDescAlloca) :
		m_RefCount(0),
		m_DescriptorHandle(INVALID_CPU_GPU_DESCRIPTOR_HANDLE),
		m_pDescAlloca(pDescAlloca)
	{
		// clear before use
		::memset(&m_SamplerDesc, 0, sizeof(m_SamplerDesc));
	}

	CSamplerState::~CSamplerState()
	{
		Invalidate();
	}
}