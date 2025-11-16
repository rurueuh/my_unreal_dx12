#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <stdexcept>

class ShadowMap
{
public:
    void Initialize(ID3D12Device* device,
        UINT width, UINT height,
        D3D12_CPU_DESCRIPTOR_HANDLE srvCpu,
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpu)
    {
        m_width = width;
        m_height = height;

        m_srvCPU = srvCpu;
        m_srvGPU = srvGpu;

        DXGI_FORMAT texFormat = DXGI_FORMAT_R32_TYPELESS;
        DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;
        DXGI_FORMAT srvFormat = DXGI_FORMAT_R32_FLOAT;

        D3D12_CLEAR_VALUE clear{};
        clear.Format = dsvFormat;
        clear.DepthStencil.Depth = 1.0f;
        clear.DepthStencil.Stencil = 0;

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC texDesc{};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Alignment = 0;
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = texFormat;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        if (FAILED(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            &clear,
            IID_PPV_ARGS(&m_tex))))
        {
            throw std::runtime_error("CreateCommittedResource (shadow map) failed");
        }

        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        dsvHeapDesc.NodeMask = 0;

        if (FAILED(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap))))
        {
            throw std::runtime_error("CreateDescriptorHeap DSV (shadow) failed");
        }

        m_dsvCPU = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = dsvFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        dsvDesc.Texture2D.MipSlice = 0;

        device->CreateDepthStencilView(m_tex.Get(), &dsvDesc, m_dsvCPU);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = srvFormat;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        device->CreateShaderResourceView(m_tex.Get(), &srvDesc, m_srvCPU);

        m_viewport.TopLeftX = 0.0f;
        m_viewport.TopLeftY = 0.0f;
        m_viewport.Width = static_cast<float>(width);
        m_viewport.Height = static_cast<float>(height);
        m_viewport.MinDepth = 0.0f;
        m_viewport.MaxDepth = 1.0f;

        m_scissor.left = 0;
        m_scissor.top = 0;
        m_scissor.right = static_cast<LONG>(width);
        m_scissor.bottom = static_cast<LONG>(height);
    }

    ID3D12Resource* Resource() const { return m_tex.Get(); }

    D3D12_CPU_DESCRIPTOR_HANDLE DSV() const { return m_dsvCPU; }

    D3D12_GPU_DESCRIPTOR_HANDLE SRVGPU() const { return m_srvGPU; }

    const D3D12_VIEWPORT& Viewport() const { return m_viewport; }
    const D3D12_RECT& Scissor()  const { return m_scissor; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource>       m_tex;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    D3D12_CPU_DESCRIPTOR_HANDLE m_dsvCPU{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_srvCPU{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_srvGPU{};

    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT     m_scissor{};

    UINT m_width = 0;
    UINT m_height = 0;
};
