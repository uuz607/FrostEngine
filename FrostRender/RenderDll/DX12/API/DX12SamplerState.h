#pragma once

#include"DX12\Interface\FrostRender.h"
#include"DX12Base.h"
#include"DX12DescriptorHeap.h"

namespace FrostDX12
{
	class CDescriptorAllocator;

	class CSamplerState : public IFrostSamplerState
	{
	public:
		virtual void AddRef()  override;
		virtual void Release() override;

		virtual void Invalidate() override;

	public:
		CSamplerState(CDescriptorAllocator* pDescAlloca);
		~CSamplerState() noexcept;

		D3D12_SAMPLER_DESC GetSamplerDesc()
		{
			return m_SamplerDesc;
		}

		void SetSamplerDesc(const D3D12_SAMPLER_DESC& SamplerDesc)
		{
			m_SamplerDesc = SamplerDesc;
		}

		void SetDescriptorHandle(const DESCRIPTOR_HANDLE& DescriptorHandle)
		{
			m_DescriptorHandle = DescriptorHandle;
		}
		DESCRIPTOR_HANDLE GetDescriptorHandle() const
		{
			return m_DescriptorHandle;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const
		{
			return m_DescriptorHandle.CPUHandle;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle() const

		{
			return m_DescriptorHandle.GPUHandle;
		}

		void SetCPUDescriptorHandle(const D3D12_CPU_DESCRIPTOR_HANDLE& DescriptorHandle)
		{
			m_DescriptorHandle.CPUHandle = DescriptorHandle;
		}

		void SetGPUDescriptorHandle(const D3D12_GPU_DESCRIPTOR_HANDLE& DescriptorHandle)
		{
			m_DescriptorHandle.GPUHandle = DescriptorHandle;
		}


	private:
		DESCRIPTOR_HANDLE m_DescriptorHandle;

		CDescriptorAllocator* m_pDescAlloca;

		D3D12_SAMPLER_DESC m_SamplerDesc;

		volatile long m_RefCount;
	};
}