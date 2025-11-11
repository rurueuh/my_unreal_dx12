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

// for revert triangle winding order (ABC -> ACB) useful for backface culling
inline static void MakeCW(std::vector<uint32_t>& idx)
{
    for (size_t i = 0; i + 2 < idx.size(); i += 3)
        std::swap(idx[i + 1], idx[i + 2]);
}

Mesh Mesh::CreatePlane(float width, float depth, uint32_t m, uint32_t n)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    vertices.resize(m * n);
    for (uint32_t i = 0; i < m; ++i) {
        float x = -0.5f * width + i * (width / (m - 1));
        for (uint32_t j = 0; j < n; ++j) {
            float z = -0.5f * depth + j * (depth / (n - 1));
            Vertex& v = vertices[i * n + j];
            v.px = x;
            v.py = 0.0f;
            v.pz = z;
            v.nx = 0.0f;
            v.ny = 1.0f;
            v.nz = 0.0f;
            v.r = 1.0f;
            v.g = 1.0f;
            v.b = 1.0f;
            v.u = static_cast<float>(i) / (m - 1);
            v.v = static_cast<float>(j) / (n - 1);
        }
    }
    for (uint32_t i = 0; i < m - 1; ++i) {
        for (uint32_t j = 0; j < n - 1; ++j) {
            indices.push_back(i * n + j);
            indices.push_back((i + 1) * n + (j + 1));
            indices.push_back((i + 1) * n + j);
            indices.push_back(i * n + j);
            indices.push_back(i * n + (j + 1));
            indices.push_back((i + 1) * n + (j + 1));
        }
    }
	return Mesh(vertices, indices);
}

Mesh Mesh::CreateCube(float size)
{
    const float s = size * 0.5f;

    std::vector<Vertex> vertices = {
        { -s, -s,  s,  0, 0, 1,  1, 1, 1,  0, 0 },
        {  s, -s,  s,  0, 0, 1,  1, 1, 1,  1, 0 },
        {  s,  s,  s,  0, 0, 1,  1, 1, 1,  1, 1 },
        { -s,  s,  s,  0, 0, 1,  1, 1, 1,  0, 1 },

        {  s, -s, -s,  0, 0,-1,  1, 1, 1,  0, 0 },
        { -s, -s, -s,  0, 0,-1,  1, 1, 1,  1, 0 },
        { -s,  s, -s,  0, 0,-1,  1, 1, 1,  1, 1 },
        {  s,  s, -s,  0, 0,-1,  1, 1, 1,  0, 1 },

        { -s,  s,  s,  0, 1, 0,  1, 1, 1,  0, 0 },
        {  s,  s,  s,  0, 1, 0,  1, 1, 1,  1, 0 },
        {  s,  s, -s,  0, 1, 0,  1, 1, 1,  1, 1 },
        { -s,  s, -s,  0, 1, 0,  1, 1, 1,  0, 1 },

        { -s, -s, -s,  0,-1, 0,  1, 1, 1,  0, 0 },
        {  s, -s, -s,  0,-1, 0,  1, 1, 1,  1, 0 },
        {  s, -s,  s,  0,-1, 0,  1, 1, 1,  1, 1 },
        { -s, -s,  s,  0,-1, 0,  1, 1, 1,  0, 1 },

        {  s, -s,  s,  1, 0, 0,  1, 1, 1,  0, 0 },
        {  s, -s, -s,  1, 0, 0,  1, 1, 1,  1, 0 },
        {  s,  s, -s,  1, 0, 0,  1, 1, 1,  1, 1 },
        {  s,  s,  s,  1, 0, 0,  1, 1, 1,  0, 1 },

        { -s, -s, -s, -1, 0, 0,  1, 1, 1,  0, 0 },
        { -s, -s,  s, -1, 0, 0,  1, 1, 1,  1, 0 },
        { -s,  s,  s, -1, 0, 0,  1, 1, 1,  1, 1 },
        { -s,  s, -s, -1, 0, 0,  1, 1, 1,  0, 1 },
    };

    std::vector<uint32_t> indices;
    for (uint32_t f = 0; f < 6; ++f) {
        uint32_t i = f * 4;
        indices.push_back(i + 0);
        indices.push_back(i + 1);
        indices.push_back(i + 2);
        indices.push_back(i + 0);
        indices.push_back(i + 2);
        indices.push_back(i + 3);
    }

    return Mesh(vertices, indices);
}

Mesh Mesh::CreateSphere(float diameter, uint16_t slices, uint16_t stacks)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    float r = diameter * 0.5f;

    for (uint32_t i = 0; i <= stacks; ++i) {
        float phi = M_PI * i / stacks;
        for (uint32_t j = 0; j <= slices; ++j) {
            float theta = 2 * M_PI * j / slices;
            float x = sinf(phi) * cosf(theta);
            float y = cosf(phi);
            float z = sinf(phi) * sinf(theta);
            vertices.push_back({ x * r, y * r, z * r, x, y, z, 1, 1, 1, (float)j / slices, (float)i / stacks });
        }
    }

    for (uint32_t i = 0; i < stacks; ++i) {
        for (uint32_t j = 0; j < slices; ++j) {
            uint32_t a = i * (slices + 1) + j;
            uint32_t b = a + slices + 1;
            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(a + 1);
            indices.push_back(a + 1);
            indices.push_back(b);
            indices.push_back(b + 1);
        }
    }
    MakeCW(indices);

    return Mesh(vertices, indices);
}

Mesh Mesh::CreateCylinder(float radius, float height, uint32_t slices, bool withCaps)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    const float h = height * 0.5f;
    const float invS = 1.0f / slices;

    for (uint32_t i = 0; i <= slices; ++i) {
        float t = i * invS;
        float th = 2.0f * float(M_PI) * t;
        float x = std::cos(th), z = std::sin(th);

        Vertex vb{ radius * x, -h, radius * z,  x, 0, z,  1,1,1,  t, 0.0f };
        Vertex vt{ radius * x,  h, radius * z,  x, 0, z,  1,1,1,  t, 1.0f };
        vertices.push_back(vb);
        vertices.push_back(vt);
    }

    for (uint32_t i = 0; i < slices; ++i) {
        uint32_t bi = 2 * i;
        uint32_t ti = 2 * i + 1;
        uint32_t bn = 2 * (i + 1);
        uint32_t tn = 2 * (i + 1) + 1;
        
        indices.push_back(bi); indices.push_back(ti); indices.push_back(tn);
        indices.push_back(bi); indices.push_back(tn); indices.push_back(bn);
    }

    if (withCaps) {
        uint32_t baseCenter = (uint32_t)vertices.size();
        vertices.push_back({0,-h,0,0,-1,0,1,1,1,0.5f,0.5f});

        uint32_t baseStart = (uint32_t)vertices.size();
        for (uint32_t i = 0; i <= slices; ++i) {
            float t = i * invS;
            float th = 2.0f * float(M_PI) * t;
            float x = std::cos(th), z = std::sin(th);
            vertices.push_back({ radius * x, -h, radius * z,  0,-1,0,  1,1,1,  0.5f * (x + 1), 0.5f * (z + 1) });
        }
        for (uint32_t i = 0; i < slices; ++i) {
            indices.push_back(baseCenter);
            indices.push_back(baseStart + i);
            indices.push_back(baseStart + i + 1);
        }

        uint32_t topCenter = (uint32_t)vertices.size();
        vertices.push_back({0,h,0,0,1,0,1,1,1,0.5f,0.5f});

        uint32_t topStart = (uint32_t)vertices.size();
        for (uint32_t i = 0; i <= slices; ++i) {
            float t = i * invS;
            float th = 2.0f * float(M_PI) * t;
            float x = std::cos(th), z = std::sin(th);
            vertices.push_back({ radius * x,  h, radius * z,  0, 1,0,  1,1,1,  0.5f * (x + 1), 0.5f * (z + 1) });
        }
        for (uint32_t i = 0; i < slices; ++i) {
            indices.push_back(topCenter);
            indices.push_back(topStart + i + 1);
            indices.push_back(topStart + i);
        }
    }

    return Mesh(vertices, indices);
}

Mesh Mesh::CreateCone(float radius, float height, uint32_t slices, bool withBase)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    const float h = height * 0.5f;
    const float invSlices = 1.0f / slices;

    const float denom = std::sqrt(radius * radius + height * height);
    const float ny_side = radius / denom;
    const float k_rad = height / denom;

    for (uint32_t i = 0; i <= slices; ++i) {
        float t = i * invSlices;
        float theta = 2.0f * float(M_PI) * t;
        float x = std::cos(theta);
        float z = std::sin(theta);
        
        float nx = x * k_rad;
        float nz = z * k_rad;

        vertices.push_back({
            radius * x, -h, radius * z,
            nx, ny_side, nz,
            1,1,1,
            t, 0.0f
        });

        vertices.push_back({
            0.0f, +h, 0.0f,
            nx, ny_side, nz,
            1,1,1,
            t, 1.0f
        });
    }

    for (uint32_t i = 0; i < slices; ++i) {
        uint32_t bottom_i = 2 * i;
        uint32_t apex_i = 2 * i + 1;
        uint32_t bottom_n = 2 * (i + 1);
        indices.push_back(apex_i);
        indices.push_back(bottom_i);
        indices.push_back(bottom_n);
    }

    if (withBase) {
        uint32_t baseCenter = (uint32_t)vertices.size();
        vertices.push_back({
            0.0f, -h, 0.0f,
            0.0f,-1.0f,0.0f,
            1,1,1,
            0.5f, 0.5f
            });

        uint32_t baseStart = (uint32_t)vertices.size();
        for (uint32_t i = 0; i <= slices; ++i) {
            float t = i * invSlices;
            float theta = 2.0f * float(M_PI) * t;
            float x = std::cos(theta);
            float z = std::sin(theta);
            vertices.push_back({
                radius * x, -h, radius * z,
                0.0f,-1.0f,0.0f,
                1,1,1,
                0.5f * (x + 1.0f), 0.5f * (z + 1.0f)
                });
        }

        for (uint32_t i = 0; i < slices; ++i) {
            indices.push_back(baseCenter);
            indices.push_back(baseStart + i + 1);
            indices.push_back(baseStart + i);
        }
    }

	MakeCW(indices);
    return Mesh(vertices, indices);
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
