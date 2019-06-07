#pragma once

#include"DX12\Interface\FrostRender.h"
#include"DX12Base.h"

namespace FrostDX12
{
	class CResource;
	class CDescriptorAllocator;

	enum VIEW_TYPE : UINT8
	{
		VIEW_TYPE_UNKNOWN,
		VIEW_TYPE_VERTEX_BUFFER,
		VIEW_TYPE_INDEX_BUFFER,
		VIEW_TYPE_CONSTANT_BUFFER,
		VIEW_TYPE_SHADER_RESOURCE,
		VIEW_TYPE_UNORDERED_ACCESS,
		VIEW_TYPE_DEPTH_STENCIL,
		VIEW_TYPE_RENDER_TARGET
	};

	class CView : public IFrostView
	{
	public:
		virtual void AddRef()  override;
		virtual void Release() override;

		virtual void Invalidate() override;
	public:
		CView();
		CView(CDescriptorAllocator* pDescAlloca);
		~CView() noexcept;

#pragma region Get() & Set() & Is()
		ID3D12Resource* GetD3D12Resource() const;
		CResource*		GetDX12Resource()  const { return m_pResource; }

		void SetType(VIEW_TYPE Type) { m_Type = Type; }
		VIEW_TYPE GetType() const	 { return m_Type; }

		void HasDesc(bool has) { m_bDesc = has; }
		bool HasDesc() const   { return m_bDesc; }

		void HasResource(bool has) { m_bResource = has; }
		bool HasResource() const   { return m_bResource; }

		const D3D12_DEPTH_STENCIL_VIEW_DESC&    GetDSVDesc() const { return m_DSVDesc; }
		const D3D12_RENDER_TARGET_VIEW_DESC&	GetRTVDesc() const { return m_RTVDesc; }
		const D3D12_VERTEX_BUFFER_VIEW&		    GetVBVDesc() const { return m_VBVDesc; }
		const D3D12_INDEX_BUFFER_VIEW&		    GetIBVDesc() const { return m_IBVDesc; }
		const D3D12_CONSTANT_BUFFER_VIEW_DESC&  GetCBVDesc() const { return m_CBVDesc; }
		const D3D12_SHADER_RESOURCE_VIEW_DESC&  GetSRVDesc() const { return m_SRVDesc; }
		const D3D12_UNORDERED_ACCESS_VIEW_DESC& GetUAVDesc() const { return m_UAVDesc; }

		void SetDescriptorHandle(const DESCRIPTOR_HANDLE DescriptorHandle) 
		{ 
			m_DescriptorHandle = DescriptorHandle;
		
		}

		DESCRIPTOR_HANDLE GetDescriptorHandle() const 
		{ 
			return m_DescriptorHandle; 
		}

		void SetCPUDescriptorHandle(const D3D12_CPU_DESCRIPTOR_HANDLE& DescriptorHandle) 
		{ 
			m_DescriptorHandle.CPUHandle = DescriptorHandle;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const 
		{ 
			return m_DescriptorHandle.CPUHandle; 
		}

		void SetGPUDescriptorHandle(const D3D12_GPU_DESCRIPTOR_HANDLE& DescriptorHandle) 
		{ 
			m_DescriptorHandle.GPUHandle = DescriptorHandle;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle() const 
		{ 
			return m_DescriptorHandle.GPUHandle;
		}
#pragma endregion

		void CreateVBV(const D3D12_VERTEX_BUFFER_VIEW& ViewDesc, IFrostResource* pResource);
		void CreateIBV(const D3D12_INDEX_BUFFER_VIEW& ViewDesc,  IFrostResource* pResource);
		void CreateSRV(const D3D12_SHADER_RESOURCE_VIEW_DESC*  ViewDesc, IFrostResource* pResource);
		void CreateCBV(const D3D12_CONSTANT_BUFFER_VIEW_DESC* ViewDesc,  IFrostResource* pResource);
		void CreateUAV(const D3D12_UNORDERED_ACCESS_VIEW_DESC* ViewDesc, IFrostResource* pResource);
		void CreateDSV(const D3D12_DEPTH_STENCIL_VIEW_DESC* ViewDesc, IFrostResource* pResource);
		void CreateRTV(const D3D12_RENDER_TARGET_VIEW_DESC* ViewDesc, IFrostResource* pResource);

	private:
		CResource* m_pResource;

		CDescriptorAllocator* m_pDescAlloca;

		DESCRIPTOR_HANDLE m_DescriptorHandle;

		union 
		{
			D3D12_VERTEX_BUFFER_VIEW         m_VBVDesc;
			D3D12_INDEX_BUFFER_VIEW          m_IBVDesc;
			D3D12_CONSTANT_BUFFER_VIEW_DESC  m_CBVDesc;
			D3D12_SHADER_RESOURCE_VIEW_DESC  m_SRVDesc;
			D3D12_UNORDERED_ACCESS_VIEW_DESC m_UAVDesc;
			D3D12_DEPTH_STENCIL_VIEW_DESC    m_DSVDesc;
			D3D12_RENDER_TARGET_VIEW_DESC    m_RTVDesc;
		};

		VIEW_TYPE  m_Type;

		volatile long m_RefCount;

		// Some views can be created without descriptor (DSV)
		bool  m_bDesc;
		// Some views can be created without resources (null SRV)
		bool  m_bResource;
	};
}