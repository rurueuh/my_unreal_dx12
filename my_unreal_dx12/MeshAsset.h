#pragma once
#include <memory>
#include <vector>
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include "Texture.h"

struct Vertex {
    float px, py, pz;
    float nx, ny, nz;
    float r, g, b;
    float u, v;
};

struct Submesh
{
    uint32_t indexStart = 0;
    uint32_t indexCount = 0;

    DirectX::XMFLOAT3 kd{ 1.f, 1.f, 1.f };
    float shininess = 128.f;

    std::shared_ptr<Texture> texture;
};

class MeshAsset
{
public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    float shininess = 128.f;

    Microsoft::WRL::ComPtr<ID3D12Resource> vb, ib;
    D3D12_VERTEX_BUFFER_VIEW vbv{};
    D3D12_INDEX_BUFFER_VIEW ibv{};
    UINT indexCount = 0;

    std::shared_ptr<Texture> texture;

    std::vector<Submesh> submeshes;

    void Upload(ID3D12Device* device);
};