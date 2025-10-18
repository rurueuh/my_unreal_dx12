#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <vector>
#include <string>
#include "Utils.h"
#include <exception>
#include "GraphicsDevice.h"

using Microsoft::WRL::ComPtr;

class GraphicsDevice;

class Texture {
public:
    void LoadFromFile(GraphicsDevice &gd, const char* path);

    ID3D12DescriptorHeap* SRVHeap() const { return m_srvHeap.Get(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle() const { return m_srvHeap->GetGPUDescriptorHandleForHeapStart(); }

private:
    ComPtr<ID3D12Resource> m_tex;
    ComPtr<ID3D12Resource> m_upload;
    ComPtr<ID3D12DescriptorHeap> m_srvHeap;
};
