#include "Mesh.h"
#include "WindowDX12.h"
using namespace DirectX;

static inline void NormalizeSafe(XMVECTOR& q) {
    if (XMVector4Less(XMVector4LengthSq(q), XMVectorReplicate(1e-20f))) {
        q = XMQuaternionIdentity();
    }
    else {
        q = XMQuaternionNormalize(q);
    }
}
static inline float DegToRad(float d) { return XMConvertToRadians(d); }

static inline float WrapDeg(float a) {
    a = std::fmod(a + 180.f, 360.f);
    if (a < 0.f) a += 360.f;
    return a - 180.f;
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
	this->setShininess(m_asset->shininess);
    m_asset->Upload(WindowDX12::Get().GetDevice());
    RecomputeRotationFromAbsoluteEuler();
}

Mesh::Mesh(const std::string& filename) {
    m_asset = ResourceCache::I().getMeshFromOBJ(filename);
    if (!m_asset->texture) m_asset->texture = ResourceCache::I().defaultWhite();
    RecomputeRotationFromAbsoluteEuler();
}

void Mesh::SetColor(float r, float g, float b) {
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

void Mesh::SetPositionX(float x)
{
    XMFLOAT3 p; XMStoreFloat3(&p, m_position);
    p.x = x;
    m_position = XMVectorSet(p.x, p.y, p.z, 0.0f);
	UpdateMatrix();
}

void Mesh::SetPositionY(float y)
{
    XMFLOAT3 p; XMStoreFloat3(&p, m_position);
    p.y = y;
    m_position = XMVectorSet(p.x, p.y, p.z, 0.0f);
    UpdateMatrix();
}
void Mesh::SetPositionZ(float z)
{
    XMFLOAT3 p; XMStoreFloat3(&p, m_position);
    p.z = z;
    m_position = XMVectorSet(p.x, p.y, p.z, 0.0f);
    UpdateMatrix();
}


void Mesh::AddPosition(float dx, float dy, float dz) {
    m_position = XMVectorAdd(m_position, XMVectorSet(dx, dy, dz, 0.0f));
    UpdateMatrix();
}

void Mesh::AddPositionX(float dx)
{
    XMFLOAT3 p; XMStoreFloat3(&p, m_position);
    p.x += dx;
    m_position = XMVectorSet(p.x, p.y, p.z, 0.0f);
    UpdateMatrix();
}
void Mesh::AddPositionY(float dy)
{
    XMFLOAT3 p; XMStoreFloat3(&p, m_position);
    p.y += dy;
    m_position = XMVectorSet(p.x, p.y, p.z, 0.0f);
    UpdateMatrix();
}
void Mesh::AddPositionZ(float dz)
{
    XMFLOAT3 p; XMStoreFloat3(&p, m_position);
    p.z += dz;
    m_position = XMVectorSet(p.x, p.y, p.z, 0.0f);
    UpdateMatrix();
}

void Mesh::RecomputeRotationFromAbsoluteEuler()
{
    m_yawDeg = WrapDeg(m_yawDeg);
    m_pitchDeg = WrapDeg(m_pitchDeg);
    m_rollDeg = WrapDeg(m_rollDeg);

    const XMVECTOR qy = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), DegToRad(m_yawDeg));
    const XMVECTOR qx = XMQuaternionRotationAxis(XMVectorSet(1.f, 0.f, 0.f, 0.f), DegToRad(m_pitchDeg));
    const XMVECTOR qz = XMQuaternionRotationAxis(XMVectorSet(0.f, 0.f, 1.f, 0.f), DegToRad(m_rollDeg));

    XMVECTOR q = XMQuaternionMultiply(qy, XMQuaternionMultiply(qx, qz));
    NormalizeSafe(q);
    m_rotQ = q;

    UpdateMatrix();
}

void Mesh::SetRotationYawPitchRoll(float yawDeg, float pitchDeg, float rollDeg) {
    m_yawDeg = yawDeg;
    m_pitchDeg = pitchDeg;
    m_rollDeg = rollDeg;
    RecomputeRotationFromAbsoluteEuler();
}

void Mesh::SetRotationYaw(float yawDeg) {
    m_yawDeg = yawDeg;
    RecomputeRotationFromAbsoluteEuler();
}

void Mesh::SetRotationPitch(float pitchDeg) {
    m_pitchDeg = pitchDeg;
    RecomputeRotationFromAbsoluteEuler();
}

void Mesh::SetRotationRoll(float rollDeg) {
    m_rollDeg = rollDeg;
    RecomputeRotationFromAbsoluteEuler();
}

void Mesh::AddRotationYaw(float dyawDeg) {
    m_yawDeg = WrapDeg(m_yawDeg + dyawDeg);
    RecomputeRotationFromAbsoluteEuler();
}

void Mesh::AddRotationPitch(float dpitchDeg) {
    m_pitchDeg = WrapDeg(m_pitchDeg + dpitchDeg);
    RecomputeRotationFromAbsoluteEuler();
}

void Mesh::AddRotationRoll(float drollDeg) {
    m_rollDeg = WrapDeg(m_rollDeg + drollDeg);
    RecomputeRotationFromAbsoluteEuler();
}

void Mesh::AddRotationYawPitchRoll(float dyawDeg, float dpitchDeg, float drollDeg) {
    m_yawDeg = WrapDeg(m_yawDeg + dyawDeg);
    m_pitchDeg = WrapDeg(m_pitchDeg + dpitchDeg);
    m_rollDeg = WrapDeg(m_rollDeg + drollDeg);
    RecomputeRotationFromAbsoluteEuler();
}

void Mesh::SetScale(float sx, float sy, float sz) {
    m_scale = XMVectorSet(sx, sy, sz, 0.0f);
    UpdateMatrix();
}
void Mesh::SetScaleX(float sx)
{
    XMFLOAT3 s; XMStoreFloat3(&s, m_scale);
    s.x = sx;
    if (s.x == 0) s.x = 1e-6f;
    m_scale = XMVectorSet(s.x, s.y, s.z, 0.0f);
	UpdateMatrix();
}

void Mesh::SetScaleY(float sy)
{
    XMFLOAT3 s; XMStoreFloat3(&s, m_scale);
    s.y = sy;
    if (s.y == 0) s.y = 1e-6f;
    m_scale = XMVectorSet(s.x, s.y, s.z, 0.0f);
    UpdateMatrix();
}

void Mesh::SetScaleZ(float sz)
{
    XMFLOAT3 s; XMStoreFloat3(&s, m_scale);
    s.z = sz;
    if (s.z == 0) s.z = 1e-6f;
    m_scale = XMVectorSet(s.x, s.y, s.z, 0.0f);
    UpdateMatrix();
}

void Mesh::AddScaleX(float dsx)
{
    XMFLOAT3 s; XMStoreFloat3(&s, m_scale);
    s.x += dsx;
    if (s.x == 0) s.x = 1e-6f;
    m_scale = XMVectorSet(s.x, s.y, s.z, 0.0f);
    UpdateMatrix();
}

void Mesh::AddScaleY(float dsy)
{
    XMFLOAT3 s; XMStoreFloat3(&s, m_scale);
    s.y += dsy;
    if (s.y == 0) s.y = 1e-6f;
    m_scale = XMVectorSet(s.x, s.y, s.z, 0.0f);
    UpdateMatrix();
}

void Mesh::AddScaleZ(float dsz)
{
    XMFLOAT3 s; XMStoreFloat3(&s, m_scale);
    s.z += dsz;
    if (s.z == 0) s.z = 1e-6f;
    m_scale = XMVectorSet(s.x, s.y, s.z, 0.0f);
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
    cmdList->SetGraphicsRootDescriptorTable(rootParamIndex, tx->GPUHandle());
}
