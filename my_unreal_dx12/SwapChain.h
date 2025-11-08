#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include "Utils.h"
#include "GraphicsDevice.h"

class SwapChain
{
public:
    void Create(GraphicsDevice& gd, HWND hwnd, UINT width, UINT height)
    {
        m_width = width; m_height = height;
        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.BufferCount = kSwapBufferCount;
        desc.Width = width;
        desc.Height = height;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.SampleDesc = { 1, 0 };

        ComPtr<IDXGISwapChain1> sc1;
        DXThrow(gd.Factory()->CreateSwapChainForHwnd(gd.Queue(), hwnd, &desc, nullptr, nullptr, &sc1));
        DXThrow(sc1.As(&m_swapchain));
        m_frameIndex = m_swapchain->GetCurrentBackBufferIndex();

        // RTV heap
        D3D12_DESCRIPTOR_HEAP_DESC rtv{};
        rtv.NumDescriptors = kSwapBufferCount;
        rtv.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        DXThrow(gd.Device()->CreateDescriptorHeap(&rtv, IID_PPV_ARGS(&m_rtvHeap)));
        m_rtvStride = gd.Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        auto handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < kSwapBufferCount; ++i)
        {
            DXThrow(m_swapchain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i])));
            gd.Device()->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, handle);
            handle.ptr += m_rtvStride;
        }
    }

    void Resize(GraphicsDevice& gd, UINT width, UINT height)
    {
        if (width == 0 || height == 0) return; // minimized
        m_width = width; m_height = height;
        for (UINT i = 0; i < kSwapBufferCount; ++i) m_backBuffers[i].Reset();
        DXThrow(m_swapchain->ResizeBuffers(kSwapBufferCount, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
        m_frameIndex = m_swapchain->GetCurrentBackBufferIndex();
        auto handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < kSwapBufferCount; ++i)
        {
            DXThrow(m_swapchain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i])));
            gd.Device()->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, handle);
            handle.ptr += m_rtvStride;
        }
    }

    IDXGISwapChain3* Swap() const { return m_swapchain.Get(); }
    ID3D12Resource* BackBuffer(UINT i) const { return m_backBuffers[i].Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentRTV() const
    {
        auto h = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        h.ptr += m_frameIndex * m_rtvStride; return h;
    }
    UINT  Width() const { return m_width; }
    UINT  Height() const { return m_height; }
    UINT  FrameIndex() const { return m_frameIndex; }
    void  UpdateFrameIndex() { m_frameIndex = m_swapchain->GetCurrentBackBufferIndex(); }
    UINT BufferCount() const { return kSwapBufferCount; }

private:
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapchain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_backBuffers[kSwapBufferCount];
    UINT m_rtvStride = 0; UINT m_frameIndex = 0; UINT m_width = 0, m_height = 0;
};