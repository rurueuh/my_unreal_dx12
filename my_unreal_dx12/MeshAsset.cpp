#include "MeshAsset.h"
#include "WindowDX12.h"
#include "Utils.h"

void MeshAsset::Upload(ID3D12Device* device) {
    if (!device) device = WindowDX12::Get().GetDevice();

    const UINT vbBytes = UINT(m_vertices.size() * sizeof(Vertex));
    const UINT ibBytes = UINT(m_indices.size() * sizeof(uint16_t));

    auto makeBuf = [&](Microsoft::WRL::ComPtr<ID3D12Resource>& res, UINT bytes) {
        if (res && res->GetDesc().Width >= bytes) return;
        D3D12_HEAP_PROPERTIES hp{}; hp.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC rd{}; rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        rd.Width = bytes ? bytes : 1; rd.Height = 1; rd.DepthOrArraySize = 1;
        rd.MipLevels = 1; rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; rd.SampleDesc = { 1,0 };
        DXThrow(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&res)));
        };

    makeBuf(m_vb, vbBytes);
    makeBuf(m_ib, ibBytes);

    if (vbBytes) {
        void* p = nullptr; D3D12_RANGE r{ 0,0 };
        m_vb->Map(0, &r, &p);
        memcpy(p, m_vertices.data(), vbBytes);
        D3D12_RANGE w{ 0, vbBytes }; m_vb->Unmap(0, &w);
    }
    if (ibBytes) {
        void* p = nullptr; D3D12_RANGE r{ 0,0 };
        m_ib->Map(0, &r, &p);
        memcpy(p, m_indices.data(), ibBytes);
        D3D12_RANGE w{ 0, ibBytes }; m_ib->Unmap(0, &w);
    }

    m_vbv.BufferLocation = m_vb->GetGPUVirtualAddress();
    m_vbv.StrideInBytes = sizeof(Vertex);
    m_vbv.SizeInBytes = vbBytes;

    m_ibv.BufferLocation = m_ib->GetGPUVirtualAddress();
    m_ibv.Format = DXGI_FORMAT_R16_UINT;
    m_ibv.SizeInBytes = ibBytes;

    m_indexCount = UINT(m_indices.size());
}