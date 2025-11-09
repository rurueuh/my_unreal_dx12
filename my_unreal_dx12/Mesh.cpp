#include "Mesh.h"
#include "WindowDX12.h"
using namespace DirectX;

static inline float Deg(float d) { return XMConvertToRadians(d); }

static inline void NormalizeSafe(XMVECTOR& q) {
    if (XMVector4Less(XMVector4LengthSq(q), XMVectorReplicate(1e-20f))) {
        q = XMQuaternionIdentity();
    }
    else {
        q = XMQuaternionNormalize(q);
    }
}

void Mesh::UpdateMatrix() {
    const XMMATRIX S = XMMatrixScalingFromVector(m_scale);
    const XMMATRIX R = XMMatrixRotationQuaternion(m_rotQ);
    const XMMATRIX T = XMMatrixTranslationFromVector(m_position);
    m_transform = S * R * T;
}

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
    m_asset = std::make_shared<MeshAsset>();
    m_asset->vertices = vertices;
    m_asset->indices = indices;
    m_asset->texture = ResourceCache::I().defaultWhite();
    m_asset->Upload(WindowDX12::Get().GetDevice());
    UpdateMatrix();
}

Mesh::Mesh(const std::string& filename) {
    m_asset = ResourceCache::I().getMeshFromOBJ(filename);
    if (!m_asset->texture) m_asset->texture = ResourceCache::I().defaultWhite();
    UpdateMatrix();
}

void Mesh::setColor(float r, float g, float b) {
    for (auto& v : m_asset->vertices) { v.r = r; v.g = g; v.b = b; }
    m_asset->Upload(nullptr);
}
std::tuple<float, float, float> Mesh::getColor() const {
    if (m_asset->vertices.empty()) return { 1.f,1.f,1.f };
    const auto& v = m_asset->vertices[0];
    return { v.r, v.g, v.b };
}

void Mesh::SetPosition(float x, float y, float z) {
    m_position = XMVectorSet(x, y, z, 0.0f);
    UpdateMatrix();
}
void Mesh::AddPosition(float dx, float dy, float dz) {
    m_position = XMVectorAdd(m_position, XMVectorSet(dx, dy, dz, 0.0f));
    UpdateMatrix();
}

void Mesh::SetRotationYawPitchRoll(float yawDeg, float pitchDeg, float rollDeg) {
    m_rotQ = XMQuaternionRotationRollPitchYaw(Deg(pitchDeg), Deg(yawDeg), Deg(rollDeg));
    NormalizeSafe(m_rotQ);
    UpdateMatrix();
}

void Mesh::AddRotationYawPitchRoll(float dyawDeg, float dpitchDeg, float drollDeg) {
    const XMVECTOR dq = XMQuaternionRotationRollPitchYaw(Deg(dpitchDeg), Deg(dyawDeg), Deg(drollDeg));
    m_rotQ = XMQuaternionMultiply(m_rotQ, dq);
    NormalizeSafe(m_rotQ);
    UpdateMatrix();
}

void Mesh::SetScale(float sx, float sy, float sz) {
    m_scale = XMVectorSet(sx, sy, sz, 0.0f);
    UpdateMatrix();
}
void Mesh::AddScale(float dsx, float dsy, float dsz) {
    m_scale = XMVectorAdd(m_scale, XMVectorSet(dsx, dsy, dsz, 0.0f));
    XMFLOAT3 s; XMStoreFloat3(&s, m_scale);
    if (s.x == 0) s.x = 1e-6f;
    if (s.y == 0) s.y = 1e-6f;
    if (s.z == 0) s.z = 1e-6f;
    m_scale = XMVectorSet(s.x, s.y, s.z, 0.0f);
    UpdateMatrix();
}

void Mesh::BindTexture(ID3D12GraphicsCommandList* cmdList, UINT rootParamIndex) const {
    auto tx = m_asset->texture ? m_asset->texture : ResourceCache::I().defaultWhite();
    if (!tx) return;
    ID3D12DescriptorHeap* heaps[] = { tx->SRVHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->SetGraphicsRootDescriptorTable(rootParamIndex, tx->GPUHandle());
}
