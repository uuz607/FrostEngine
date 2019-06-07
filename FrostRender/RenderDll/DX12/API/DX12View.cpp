#include"pch.h"
#include"DX12View.h"
#include"DX12DescriptorHeap.h"
#include"DX12Resource.h"

namespace FrostDX12
{
	/*---------------Interface implemtation---------------*/
	void CView::AddRef() 
	{
		::_InterlockedIncrement(&m_RefCount);
	}

	void CView::Release() 
	{
		long refCount = ::_InterlockedDecrement(&m_RefCount);
		if (!refCount)
		{
			delete this;
		}
	}

	/*---------------CView implemtation---------------*/
	CView::CView() :
		m_pResource(nullptr),
		m_pDescAlloca(nullptr),
		m_DescriptorHandle(),
		m_Type(VIEW_TYPE_UNKNOWN),
		m_RefCount(0),
		m_bDesc(true),
		m_bResource(true)
	{
		::memset(&m_DSVDesc, 0, sizeof(m_DSVDesc));
		::memset(&m_RTVDesc, 0, sizeof(m_RTVDesc));
		::memset(&m_VBVDesc, 0, sizeof(m_VBVDesc));
		::memset(&m_CBVDesc, 0, sizeof(m_CBVDesc));
		::memset(&m_SRVDesc, 0, sizeof(m_SRVDesc));
		::memset(&m_UAVDesc, 0, sizeof(m_UAVDesc));
	}

	CView::CView(CDescriptorAllocator* pDescAlloca) :
		m_pResource(nullptr),
		m_pDescAlloca(pDescAlloca),
		m_DescriptorHandle(),
		m_Type(VIEW_TYPE_UNKNOWN),
		m_RefCount(0),
		m_bDesc(true),
		m_bResource(true)
	{
		::memset(&m_DSVDesc, 0, sizeof(m_DSVDesc));
		::memset(&m_RTVDesc, 0, sizeof(m_RTVDesc));
		::memset(&m_VBVDesc, 0, sizeof(m_VBVDesc));
		::memset(&m_CBVDesc, 0, sizeof(m_CBVDesc));
		::memset(&m_SRVDesc, 0, sizeof(m_SRVDesc));
		::memset(&m_UAVDesc, 0, sizeof(m_UAVDesc));
	}

	CView::~CView()
	{
		Invalidate();
	}

	ID3D12Resource* CView::GetD3D12Resource() const
	{ 
		return m_pResource->GetD3D12Resource(); 
	}

	void CView::CreateVBV(const D3D12_VERTEX_BUFFER_VIEW& ViewDesc, IFrostResource* pResource)
	{
		if (!pResource)
		{
			m_bResource = false;
		}

		m_VBVDesc = ViewDesc;
		m_pResource = static_cast<CResource*>(pResource);
		m_Type = VIEW_TYPE_VERTEX_BUFFER;
	}

	void CView::CreateIBV(const D3D12_INDEX_BUFFER_VIEW& ViewDesc, IFrostResource* pResource)
	{
		if (!pResource)
		{
			m_bResource = false;
		}

		m_IBVDesc = ViewDesc;
		m_pResource = static_cast<CResource*>(pResource);
		m_Type = VIEW_TYPE_INDEX_BUFFER;
	}

	void CView::CreateSRV(const D3D12_SHADER_RESOURCE_VIEW_DESC*  ViewDesc, IFrostResource* pResource)
	{
		if (!pResource)
		{
			m_bResource = false;
			m_SRVDesc = *ViewDesc;
			m_Type = VIEW_TYPE_SHADER_RESOURCE;

			return;
		}

		if (!ViewDesc)
		{
			m_bDesc = false;
			m_pResource = static_cast<CResource*>(pResource);
			m_Type = VIEW_TYPE_SHADER_RESOURCE;

			return;
		}

		m_SRVDesc = *ViewDesc;
		m_pResource = static_cast<CResource*>(pResource);
		m_Type = VIEW_TYPE_SHADER_RESOURCE;
	}

	void CView::CreateCBV(const D3D12_CONSTANT_BUFFER_VIEW_DESC* ViewDesc, IFrostResource* pResource)
	{
		if (!pResource)
		{
			m_bResource = false;
			m_CBVDesc = *ViewDesc;
			m_Type = VIEW_TYPE_CONSTANT_BUFFER;

			return;
		}

		if (!ViewDesc)
		{
			m_bDesc = false;
			m_pResource = static_cast<CResource*>(pResource);
			m_Type = VIEW_TYPE_CONSTANT_BUFFER;

			return;
		}

		m_CBVDesc = *ViewDesc;
		m_pResource = static_cast<CResource*>(pResource);
		m_Type = VIEW_TYPE_CONSTANT_BUFFER;
	}

	void CView::CreateUAV(const D3D12_UNORDERED_ACCESS_VIEW_DESC* ViewDesc, IFrostResource* pResource)
	{
		if (!pResource)
		{
			m_bResource = false;
			m_UAVDesc = *ViewDesc;
			m_Type = VIEW_TYPE_UNORDERED_ACCESS;

			return;
		}

		if (!ViewDesc)
		{
			m_bDesc = false;
			m_pResource = static_cast<CResource*>(pResource);
			m_Type = VIEW_TYPE_UNORDERED_ACCESS;

			return;
		}

		m_UAVDesc = *ViewDesc;
		m_pResource = static_cast<CResource*>(pResource);
		m_Type = VIEW_TYPE_UNORDERED_ACCESS;

	}

	void CView::CreateDSV(const D3D12_DEPTH_STENCIL_VIEW_DESC* ViewDesc, IFrostResource* pResource)
	{
		if (!pResource)
		{
			m_bResource = false;
			m_DSVDesc = *ViewDesc;
			m_Type = VIEW_TYPE_DEPTH_STENCIL;

			return;
		}

		if (!ViewDesc)
		{
			m_bDesc = false;
			m_pResource = static_cast<CResource*>(pResource);
			m_Type = VIEW_TYPE_DEPTH_STENCIL;

			return;
		}

		m_DSVDesc = *ViewDesc;
		m_pResource = static_cast<CResource*>(pResource);
		m_Type = VIEW_TYPE_DEPTH_STENCIL;
	}

	void CView::CreateRTV(const D3D12_RENDER_TARGET_VIEW_DESC* ViewDesc, IFrostResource* pResource)
	{
		if (!pResource)
		{
			m_bResource = false;
			m_RTVDesc = *ViewDesc;
			m_Type = VIEW_TYPE_RENDER_TARGET;

			return;
		}

		if (!ViewDesc)
		{
			m_bDesc = false;
			m_pResource = static_cast<CResource*>(pResource);
			m_Type = VIEW_TYPE_RENDER_TARGET;

			return;
		}

		m_RTVDesc = *ViewDesc;
		m_pResource = static_cast<CResource*>(pResource);
		m_Type = VIEW_TYPE_RENDER_TARGET;
	}

	void CView::Invalidate()
	{
		if (INVALID_CPU_DESCRIPTOR_HANDLE != m_DescriptorHandle.CPUHandle)
		{
			switch (m_Type)
			{
			case VIEW_TYPE_CONSTANT_BUFFER:
				m_pDescAlloca->InvalidateConstantBufferView(&GetCBVDesc());
				break;
				
			case VIEW_TYPE_SHADER_RESOURCE:
				m_pDescAlloca->InvalidateShaderResourceView(&GetSRVDesc(), GetD3D12Resource());
				break;

			case VIEW_TYPE_UNORDERED_ACCESS:
				m_pDescAlloca->InvalidateUnorderedAccessView(&GetUAVDesc(), GetD3D12Resource());
				break;

			case VIEW_TYPE_DEPTH_STENCIL:
				m_pDescAlloca->InvalidateDepthStencilView(&GetDSVDesc(), GetD3D12Resource());
				break;

			case VIEW_TYPE_RENDER_TARGET:
				m_pDescAlloca->InvalidateRenderTargetView(&GetRTVDesc(), GetD3D12Resource());
				break;

			default:
					break;
			}
		}

		m_pResource = nullptr;
		m_DescriptorHandle.CPUHandle = INVALID_CPU_DESCRIPTOR_HANDLE;
		m_DescriptorHandle.GPUHandle = INVALID_GPU_DESCRIPTOR_HANDLE;
	}
}
