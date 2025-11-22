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
    float tx, ty, tz;
    float bx, by, bz;
};

struct Submesh
{
    uint32_t indexStart = 0;
    uint32_t indexCount = 0;

    DirectX::XMFLOAT3 kd{ 1.f, 1.f, 1.f };
    DirectX::XMFLOAT3 ks{ 1.f, 1.f, 1.f };
    DirectX::XMFLOAT3 ke{ 0.f, 0.f, 0.f };
    float shininess = 128.f;
    float opacity = 1.f;

    std::shared_ptr<Texture> texture;
    std::shared_ptr<Texture> normalMap;
    bool hasNormalMap = false;

    std::shared_ptr<Texture> metalRoughMap;
    bool hasMetalRoughMap = false;
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

	void setShininess(float s) { shininess = s; }

    void Upload(ID3D12Device* device);
};