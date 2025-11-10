#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <cstdint>
#include <DirectXMath.h>
#include "Utils.h"
#include <memory>
#include <cstring>

struct SceneCB
{
    DirectX::XMFLOAT4X4 uModel;
    DirectX::XMFLOAT4X4 uViewProj;
    DirectX::XMFLOAT4X4 uNormalMatrix;
};
static_assert(sizeof(SceneCB) % 16 == 0, "SceneCB must be 16-byte aligned");

class ConstantBuffer
{
public:
    void Create(ID3D12Device* device, UINT sliceCount)
    {
        m_sliceSize = Align256(sizeof(SceneCB));
        m_totalSize = m_sliceSize * sliceCount;

        D3D12_HEAP_PROPERTIES heap{};
        heap.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC buf{};
        buf.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        buf.Width = m_totalSize;
        buf.Height = 1;
        buf.DepthOrArraySize = 1;
        buf.MipLevels = 1;
        buf.SampleDesc = { 1, 0 };
        buf.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        DXThrow(device->CreateCommittedResource(
            &heap, D3D12_HEAP_FLAG_NONE,
            &buf, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&m_resource)));

        D3D12_RANGE r{ 0, 0 };
        DXThrow(m_resource->Map(0, &r, reinterpret_cast<void**>(&m_mapped)));
    }

    D3D12_GPU_VIRTUAL_ADDRESS UploadSlice(UINT sliceIndex, const SceneCB& data)
    {
        std::memcpy(m_mapped + sliceIndex * m_sliceSize, &data, sizeof(data));
        return m_resource->GetGPUVirtualAddress() + sliceIndex * m_sliceSize;
    }

    UINT SliceSize() const { return m_sliceSize; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
    UINT m_sliceSize = 0;
    UINT m_totalSize = 0;
    uint8_t* m_mapped = nullptr;
};
