#include "Mesh.h"
#include "WindowDX12.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>



static std::string joinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    const char last = a.back();
    if (last == '/' || last == '\\') return a + b;
    return a + "/" + b;
}

void parseMtlFile(const std::string& mtlPath, std::unordered_map<std::string, Material>& out) {
    std::ifstream mtl(mtlPath);
    if (!mtl.is_open()) { std::cerr << "MTL introuvable: " << mtlPath << "\n"; return; }
    std::string line, tok, cur;
    while (std::getline(mtl, line)) {
        std::istringstream iss(line);
        if (!(iss >> tok)) continue;
        if (tok == "newmtl") { iss >> cur; out[cur] = Material{}; }
        else if (tok == "Kd" && !cur.empty()) { iss >> out[cur].Kd.x >> out[cur].Kd.y >> out[cur].Kd.z; }
        else if ((tok == "map_Kd") && !cur.empty()) {
            std::string tex; iss >> tex;
            out[cur].map_Kd = tex;
        }
    }
}

static void parseVtxToken(const std::string& tok, int& vi, int& ti, int& ni)
{
    vi = ti = ni = 0;
    int part = 0;
    std::string acc;
    auto flush = [&](void) {
        if (acc.empty()) { ++part; return; }
        int val = std::stoi(acc);
        if (part == 0) vi = val;
        else if (part == 1) ti = val;
        else if (part == 2) ni = val;
        acc.clear(); ++part;
    };
    for (char c : tok) { if (c == '/') flush(); else acc.push_back(c); }
    flush();

}

static uint16_t getIndexForKey(
    const std::string& key,
    std::unordered_map<std::string, uint16_t>& map,
    uint16_t& next,
    std::vector<Vertex>& outVertices,
    const std::vector<DirectX::XMFLOAT3>& positions,
    const std::vector<DirectX::XMFLOAT2>& texcoords,
    const std::vector<DirectX::XMFLOAT3>& normals)
{
    auto it = map.find(key);
    if (it != map.end()) return it->second;

    int vi = 0, ti = 0, ni = 0;
    parseVtxToken(key, vi, ti, ni);

    Vertex vert{};
    const auto& p = positions[(vi > 0 ? vi - 1 : 0)];
    vert.px = p.x; vert.py = p.y; vert.pz = p.z;
    vert.r = vert.g = vert.b = 1.0f;

    if (ti > 0) {
        const auto& t = texcoords[ti - 1];
        vert.u = t.x;
        vert.v = 1.0f - t.y;
    }
    else {
        vert.u = vert.v = 0.0f;
    }


    outVertices.push_back(vert);
    map[key] = next;
    return next++;
}

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices) {
	this->Upload(WindowDX12::Get().GetDevice(), vertices, indices);
	std::cout << "Mesh created with " << indices.size() / 3 << " triangles.\n";
}

Mesh::Mesh(const std::string& filename)
{
    const size_t slash = filename.find_last_of("/\\");
    const std::string baseDir = (slash == std::string::npos) ? std::string() : filename.substr(0, slash + 1);
    m_baseDirW = std::wstring(baseDir.begin(), baseDir.end());
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erreur: impossible d’ouvrir " << filename << std::endl;
    }

    std::vector<DirectX::XMFLOAT3> positions;
    std::vector<DirectX::XMFLOAT3> normals;
    std::vector<DirectX::XMFLOAT2> texcoords;

    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    std::string line;
    std::unordered_map<std::string, uint16_t> vertexMap;
    uint16_t nextIndex = 0;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            DirectX::XMFLOAT3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        }
        else if (type == "vt") {
            DirectX::XMFLOAT2 tex;
            iss >> tex.x >> tex.y;
            texcoords.push_back(tex);
        }
        else if (type == "vn") {
            DirectX::XMFLOAT3 n;
            iss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (type == "f") {
            std::vector<std::string> toks;
            std::string tok;
            while (iss >> tok) toks.push_back(tok);
            if (toks.size() < 3) continue;

            std::vector<uint16_t> faceIdx;
            faceIdx.reserve(toks.size());
            for (auto& t : toks) {
                uint16_t idx = getIndexForKey(t, vertexMap, nextIndex, m_vertices, positions, texcoords, normals);
                faceIdx.push_back(idx);
            }
            for (size_t i = 2; i < faceIdx.size(); ++i) {
                m_indices.push_back(faceIdx[0]);
                m_indices.push_back(faceIdx[i - 1]);
                m_indices.push_back(faceIdx[i]);
            }
        }
        else if (type == "mtllib") {
            std::string mtlfile; iss >> mtlfile;
            parseMtlFile(joinPath(baseDir, mtlfile), m_materials);
        }
        else if (type == "usemtl") {
            std::string name; iss >> name;
            auto it = m_materials.find(name);
            if (it != m_materials.end()) {
                const auto kd = it->second.Kd;
                for (auto& v : m_vertices) { v.r = kd.x; v.g = kd.y; v.b = kd.z; }

                if (!it->second.map_Kd.empty() && !m_hasLocalTexture) {
                    auto& gd = WindowDX12::Get().GetGraphicsDevice();
                    const std::string texPath = joinPath(baseDir, it->second.map_Kd);
                    try {
                        m_localTexture.LoadFromFile(gd, texPath.c_str());
                        SetTexture(&m_localTexture);
                        m_hasLocalTexture = true;
                    }
                    catch (const std::exception& e) {
                        std::cerr << "Echec texture " << texPath << " : " << e.what() << "\n";
                    }
                }
            }
        }
    }

    this->Upload(nullptr, m_vertices, m_indices);
    std::cout << "OBJ chargé: " << m_vertices.size() << " sommets, " << m_indices.size() / 3 << " faces." << std::endl;
}

void Mesh::Upload(ID3D12Device* device,
    const std::vector<Vertex>& vertices,
    const std::vector<uint16_t>& indices,
    ID3D12Fence* fence, UINT64 fenceValue)
{
    if (!device) device = WindowDX12::Get().GetDevice();

	m_vertices = vertices;
	m_indices = indices;

    if (fence && fence->GetCompletedValue() < fenceValue) {
        HANDLE e = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        fence->SetEventOnCompletion(fenceValue, e);
    }

    const UINT vbBytesReal = UINT(vertices.size() * sizeof(Vertex));
    const UINT ibBytesReal = UINT(indices.size() * sizeof(uint16_t));
    const UINT vbBytes = std::max<UINT>(vbBytesReal, 1u);
    const UINT ibBytes = std::max<UINT>(ibBytesReal, 1u);

    auto makeOrResize = [&](ComPtr<ID3D12Resource>& res, UINT bytes) {
        if (res && res->GetDesc().Width >= bytes) return;
        D3D12_HEAP_PROPERTIES hp{}; hp.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC rd{}; rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        rd.Width = bytes; rd.Height = 1; rd.DepthOrArraySize = 1; rd.MipLevels = 1;
        rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; rd.SampleDesc = { 1,0 };
        DXThrow(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&res)));
        };

    makeOrResize(m_vb, vbBytes);
    makeOrResize(m_ib, ibBytes);

    if (vbBytesReal) {
        void* p = nullptr; D3D12_RANGE rr{ 0,0 };
        m_vb->Map(0, &rr, &p);
        memcpy(p, vertices.data(), vbBytesReal);
        D3D12_RANGE wr{ 0, vbBytesReal }; m_vb->Unmap(0, &wr);
    }
    if (ibBytesReal) {
        void* p = nullptr; D3D12_RANGE rr{ 0,0 };
        m_ib->Map(0, &rr, &p);
        memcpy(p, indices.data(), ibBytesReal);
        D3D12_RANGE wr{ 0, ibBytesReal }; m_ib->Unmap(0, &wr);
    }

    m_vbv.BufferLocation = m_vb->GetGPUVirtualAddress();
    m_vbv.StrideInBytes = sizeof(Vertex);
    m_vbv.SizeInBytes = vbBytesReal;

    m_ibv.BufferLocation = m_ib->GetGPUVirtualAddress();
    m_ibv.Format = DXGI_FORMAT_R16_UINT;
    m_ibv.SizeInBytes = ibBytesReal;

    m_indexCount = UINT(indices.size());
}

void Mesh::BindTexture(ID3D12GraphicsCommandList* cmdList, UINT rootParamIndex) const {
    Texture* tx = m_texture ? m_texture : m_defaultWhite;
    std::wcout << L"Binding texture with GPU handle: " << m_defaultWhite << L"\n";
    if (m_texture == nullptr)
        printf("%p\n", m_texture);
    if (m_defaultWhite == nullptr) {
        auto window = WindowDX12::Get();
        m_defaultWhite = &window.getDefaultTexture();
    }
    if (!tx) return;
    ID3D12DescriptorHeap* heaps[] = { tx->SRVHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->SetGraphicsRootDescriptorTable(rootParamIndex, tx->GPUHandle());
}
