#include "ResourceCache.h"
#include "Mesh.h"
#include "WindowDX12.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <DirectXMath.h>
#include <iostream>

struct Material {
    DirectX::XMFLOAT3 Kd{ 1,1,1 };
    std::string map_Kd;
    float Ns{ 128.f };
};

static std::string joinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    const char last = a.back();
    if (last == '/' || last == '\\') return a + b;
    return a + "/" + b;
}

static void parseMtlFile(const std::string& mtlPath, std::unordered_map<std::string, Material>& out) {
    std::ifstream mtl(mtlPath);
    if (!mtl.is_open()) {
        std::cerr << "MTL introuvable: " << mtlPath << "\n";
        return;
    }

    std::string line, tok, cur;
    while (std::getline(mtl, line)) {
        std::istringstream iss(line);
        if (!(iss >> tok)) continue;
        if (tok == "newmtl") {
            iss >> cur;
            out[cur] = Material{};
        }
        else if (tok == "Kd" && !cur.empty()) {
            iss >> out[cur].Kd.x >> out[cur].Kd.y >> out[cur].Kd.z;
        }
        else if (tok == "map_Kd" && !cur.empty()) {
            iss >> out[cur].map_Kd;
        }
        else if (tok == "Ns" && !cur.empty()) {
            iss >> out[cur].Ns;
            if (out[cur].Ns < 16.0f)   out[cur].Ns = 16.0f;
            if (out[cur].Ns > 256.0f)  out[cur].Ns = 256.0f;
        }
    }
}

static void parseVtxToken(const std::string& tok, int& vi, int& ti, int& ni) {
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
    for (char c : tok) {
        if (c == '/') flush();
        else acc.push_back(c);
    }
    flush();
}

static uint32_t getIndexForKey(
    const std::string& token,
    const std::string& materialKey,
    const Material* material,
    std::unordered_map<std::string, uint32_t>& map,
    uint32_t& next,
    std::vector<Vertex>& outVertices,
    const std::vector<DirectX::XMFLOAT3>& positions,
    const std::vector<DirectX::XMFLOAT2>& texcoords,
    const std::vector<DirectX::XMFLOAT3>& normals)
{
    std::string combinedKey = token;
    combinedKey.push_back('|');
    combinedKey += materialKey;

    auto it = map.find(combinedKey);
    if (it != map.end()) return it->second;

    int vi = 0, ti = 0, ni = 0;
    parseVtxToken(token, vi, ti, ni);

    Vertex vert{};
    const auto& p = positions[(vi > 0 ? vi - 1 : 0)];
    vert.px = p.x; vert.py = p.y; vert.pz = p.z;

    if (ni > 0 && (size_t)(ni - 1) < normals.size()) {
        const auto& n = normals[ni - 1];
        vert.nx = n.x; vert.ny = n.y; vert.nz = n.z;
    }
    else {
        vert.nx = 0.0f; vert.ny = 1.0f; vert.nz = 0.0f;
    }

    if (material) {
        vert.r = material->Kd.x;
        vert.g = material->Kd.y;
        vert.b = material->Kd.z;
    }
    else {
        vert.r = vert.g = vert.b = 1.0f;
    }

    if (ti > 0 && (size_t)(ti - 1) < texcoords.size()) {
        const auto& t = texcoords[ti - 1];
        vert.u = t.x;
        vert.v = 1.0f - t.y;
    }
    else {
        vert.u = vert.v = 0.0f;
    }

    outVertices.push_back(vert);
    map[combinedKey] = next;
    return next++;
}

static void RecomputeSmoothNormals(std::vector<Vertex>& verts,
    const std::vector<uint32_t>& idx)
{
    for (auto& v : verts) { v.nx = v.ny = v.nz = 0.0f; }

    for (size_t i = 0; i + 2 < idx.size(); i += 3) {
        Vertex& a = verts[idx[i]];
        Vertex& b = verts[idx[i + 1]];
        Vertex& c = verts[idx[i + 2]];

        DirectX::XMVECTOR pa = DirectX::XMVectorSet(a.px, a.py, a.pz, 0);
        DirectX::XMVECTOR pb = DirectX::XMVectorSet(b.px, b.py, b.pz, 0);
        DirectX::XMVECTOR pc = DirectX::XMVectorSet(c.px, c.py, c.pz, 0);

        DirectX::XMVECTOR ab = DirectX::XMVectorSubtract(pb, pa);
        DirectX::XMVECTOR ac = DirectX::XMVectorSubtract(pc, pa);
        DirectX::XMVECTOR n = DirectX::XMVector3Cross(ab, ac);

        DirectX::XMFLOAT3 nf;
        DirectX::XMStoreFloat3(&nf, n);
        a.nx += nf.x; a.ny += nf.y; a.nz += nf.z;
        b.nx += nf.x; b.ny += nf.y; b.nz += nf.z;
        c.nx += nf.x; c.ny += nf.y; c.nz += nf.z;
    }

    for (auto& v : verts) {
        DirectX::XMVECTOR n = DirectX::XMVectorSet(v.nx, v.ny, v.nz, 0);
        n = DirectX::XMVector3Normalize(n);
        DirectX::XMFLOAT3 nf;
        DirectX::XMStoreFloat3(&nf, n);
        v.nx = nf.x; v.ny = nf.y; v.nz = nf.z;
    }
}

void LoadOBJIntoAsset(const std::string& filename, MeshAsset& out, std::shared_ptr<Texture> defaultWhite)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: unable to open " << filename << std::endl;
        return;
    }

    const size_t slash = filename.find_last_of("/\\");
    const std::string baseDir = (slash == std::string::npos) ? "" : filename.substr(0, slash + 1);

    std::vector<DirectX::XMFLOAT3> positions;
    std::vector<DirectX::XMFLOAT3> normals;
    std::vector<DirectX::XMFLOAT2> texcoords;
    std::unordered_map<std::string, Material> materials;

    std::unordered_map<std::string, uint32_t> vertexMap;
    uint32_t nextIndex = 0;

    std::string currentMaterialName;
    const Material* currentMaterial = nullptr;
    std::unordered_map<std::string, std::shared_ptr<Texture>> materialTextures;

    auto getMaterialTexture = [&](const std::string& name) -> std::shared_ptr<Texture>
        {
            auto it = materialTextures.find(name);
            if (it != materialTextures.end())
                return it->second;

            auto matIt = materials.find(name);
            if (matIt == materials.end() || matIt->second.map_Kd.empty()) {
                materialTextures[name] = nullptr;
                return nullptr;
            }

            const std::string texPath = joinPath(baseDir, matIt->second.map_Kd);

            auto tex = std::make_shared<Texture>();
            try
            {
                auto& win = WindowDX12::Get();
                auto& gd = win.GetGraphicsDevice();
                auto  alloc = win.AllocateSrv();

                tex->LoadFromFile(gd, texPath.c_str(), alloc.cpu, alloc.gpu);

                materialTextures[name] = tex;
                return tex;
            }
            catch (...)
            {
                std::cerr << "Error texture: " << texPath << "\n";
                materialTextures[name] = nullptr;
                return nullptr;
            }
        };

    std::string line;
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

            std::vector<uint32_t> faceIdx;
            faceIdx.reserve(toks.size());
            for (auto& t : toks) {
                uint32_t idx = getIndexForKey(
                    t,
                    currentMaterialName,
                    currentMaterial,
                    vertexMap,
                    nextIndex,
                    out.vertices,
                    positions,
                    texcoords,
                    normals
                );
                faceIdx.push_back(idx);
            }

            for (size_t i = 2; i < faceIdx.size(); ++i) {
                out.indices.push_back(faceIdx[0]);
                out.indices.push_back(faceIdx[i - 1]);
                out.indices.push_back(faceIdx[i]);
            }
        }
        else if (type == "mtllib") {
            std::string mtlfile;
            iss >> mtlfile;
            parseMtlFile(joinPath(baseDir, mtlfile), materials);
        }
        else if (type == "usemtl") {
            std::string name;
            iss >> name;
            auto it = materials.find(name);
            currentMaterialName = name;
            currentMaterial = (it != materials.end()) ? &it->second : nullptr;

            if (currentMaterial) {
                out.shininess = currentMaterial->Ns;
                std::cout << "Material " << name << " Ns = " << out.shininess << "\n";
            }

            if (!out.texture)
            {
                if (auto tex = getMaterialTexture(name))
                    out.texture = tex;
                else
                    out.texture = defaultWhite;
            }
        }
    }

    if (!out.texture)
        out.texture = defaultWhite;

    const bool hasNormals = !normals.empty();
    if (!hasNormals) {
        RecomputeSmoothNormals(out.vertices, out.indices);
    }
}

std::shared_ptr<MeshAsset> ResourceCache::getMeshFromOBJ(const std::string& path) {
    std::shared_ptr<Texture> defaultWhiteCopy;
    {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = meshCache_.find(path);
        if (it != meshCache_.end()) {
            if (auto sp = it->second.lock())
                return sp;
        }
        defaultWhiteCopy = defaultWhite_;
    }

    auto asset = std::make_shared<MeshAsset>();
    LoadOBJIntoAsset(path, *asset, defaultWhiteCopy);
    asset->Upload(WindowDX12::Get().GetDevice());

    std::lock_guard<std::mutex> lk(mu_);
    auto it = meshCache_.find(path);
    if (it != meshCache_.end()) {
        if (auto sp = it->second.lock())
            return sp;
        it->second = asset;
    }
    else {
        meshCache_.emplace(path, asset);
    }
    return asset;
}
