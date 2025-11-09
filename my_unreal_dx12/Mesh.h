// Mesh.h
#pragma once
#include <memory>
#include <DirectXMath.h>
#include "MeshAsset.h"
#include "ResourceCache.h"

struct Vertex {
    float px, py, pz;
    float nx, ny, nz;
    float r, g, b;
    float u, v;
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices);
    Mesh(const std::string& filename);

    Mesh(const Mesh&) = default;
    Mesh& operator=(const Mesh&) = default;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    void SetPosition(float x, float y, float z);
    void AddPosition(float dx, float dy, float dz);
    void SetRotationYawPitchRoll(float yaw, float pitch, float roll);
    void AddRotationYawPitchRoll(float dyaw, float dpitch, float droll);
    void SetScale(float sx, float sy, float sz);
    void AddScale(float dsx, float dsy, float dsz);
    const DirectX::XMMATRIX& Transform() const { return m_transform; }
    void SetTexture(std::shared_ptr<Texture> t) { if (m_asset) m_asset->texture = std::move(t); }
    Texture* GetTexture() const {
        if (!m_asset) return nullptr;
        auto t = m_asset->texture ? m_asset->texture : ResourceCache::I().defaultWhite();
        return t ? t.get() : nullptr;
    }
    std::shared_ptr<Texture> GetTextureShared() const {
        if (!m_asset) return {};
        return m_asset->texture ? m_asset->texture : ResourceCache::I().defaultWhite();
    }

    void setColor(float r, float g, float b);
    std::tuple<float, float, float> getColor() const;

    void BindTexture(ID3D12GraphicsCommandList* cmdList, UINT rootParamIndex) const;

    const D3D12_VERTEX_BUFFER_VIEW& VBV() const { return m_asset->vbv; }
    const D3D12_INDEX_BUFFER_VIEW& IBV() const { return m_asset->ibv; }
    UINT IndexCount() const { return m_asset->indexCount; }

private:
    std::shared_ptr<MeshAsset> m_asset;
    DirectX::XMMATRIX m_transform{ DirectX::XMMatrixIdentity() };
};
