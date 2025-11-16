#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "GraphicsDevice.h"

class Texture
{
public:
    Texture() = default;

    void LoadFromFile(GraphicsDevice& gd,
        const char* path,
        D3D12_CPU_DESCRIPTOR_HANDLE srvCpu,
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpu);

    void InitWhite1x1(GraphicsDevice& gd,
        D3D12_CPU_DESCRIPTOR_HANDLE srvCpu,
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpu);

    ID3D12Resource* Resource() const { return m_tex.Get(); }

    D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle() const { return m_srvGPU; }
    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle() const { return m_srvCPU; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_tex;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_upload;

    D3D12_CPU_DESCRIPTOR_HANDLE m_srvCPU{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_srvGPU{};
};
