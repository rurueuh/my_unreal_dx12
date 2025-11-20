#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ResourceCache.h"
#include "Mesh.h"
#include "WindowDX12.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <DirectXMath.h>
#include <iostream>
#include <algorithm>



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
        else if (tok == "Ks" && !cur.empty()) {
            iss >> out[cur].Ks.x >> out[cur].Ks.y >> out[cur].Ks.z;
        }
        else if (tok == "Ke" && !cur.empty()) {
            iss >> out[cur].Ke.x >> out[cur].Ke.y >> out[cur].Ke.z;
        }
        else if (tok == "Ns" && !cur.empty()) {
            iss >> out[cur].Ns;
            if (out[cur].Ns < 16.0f)   out[cur].Ns = 16.0f;
            if (out[cur].Ns > 256.0f)  out[cur].Ns = 256.0f;
        }
        else if (tok == "d" && !cur.empty()) {
            iss >> out[cur].d;
            if (out[cur].d < 0.0f) out[cur].d = 0.0f;
            if (out[cur].d > 1.0f) out[cur].d = 1.0f;
        }
        else if (tok == "Tr" && !cur.empty()) {
            float tr;
            iss >> tr;

            tr = std::max(0.f, std::min(1.f, tr));
            out[cur].d = 1.f - tr;
        }
        else if (tok == "map_Kd" && !cur.empty()) {
            iss >> out[cur].map_Kd;
        }
        else if ((tok == "map_Bump" || tok == "bump" || tok == "map_normal") && !cur.empty())
        {
            iss >> out[cur].map_normal;
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

static void ComputeTangents(std::vector<Vertex>& verts, const std::vector<uint32_t>& idx)
{
    for (auto& v : verts) {
        v.tx = v.ty = v.tz = 0;
        v.bx = v.by = v.bz = 0;
    }

    for (size_t i = 0; i < idx.size(); i += 3) {
        Vertex& v0 = verts[idx[i]];
        Vertex& v1 = verts[idx[i + 1]];
        Vertex& v2 = verts[idx[i + 2]];

        DirectX::XMFLOAT3 p0{ v0.px, v0.py, v0.pz };
        DirectX::XMFLOAT3 p1{ v1.px, v1.py, v1.pz };
        DirectX::XMFLOAT3 p2{ v2.px, v2.py, v2.pz };

        DirectX::XMFLOAT2 uv0{ v0.u, v0.v };
        DirectX::XMFLOAT2 uv1{ v1.u, v1.v };
        DirectX::XMFLOAT2 uv2{ v2.u, v2.v };

        XMVECTOR P0 = DirectX::XMLoadFloat3(&p0);
        XMVECTOR P1 = DirectX::XMLoadFloat3(&p1);
        XMVECTOR P2 = DirectX::XMLoadFloat3(&p2);

        XMVECTOR uv0v = DirectX::XMLoadFloat2(&uv0);
        XMVECTOR uv1v = DirectX::XMLoadFloat2(&uv1);
        XMVECTOR uv2v = DirectX::XMLoadFloat2(&uv2);

        float x1 = p1.x - p0.x;
        float x2 = p2.x - p0.x;
        float y1 = p1.y - p0.y;
        float y2 = p2.y - p0.y;
        float z1 = p1.z - p0.z;
        float z2 = p2.z - p0.z;

        float s1 = uv1.x - uv0.x;
        float s2 = uv2.x - uv0.x;
        float t1 = uv1.y - uv0.y;
        float t2 = uv2.y - uv0.y;

        float r = 1.0f / (s1 * t2 - s2 * t1);

        XMFLOAT3 T{
            (t2 * x1 - t1 * x2) * r,
            (t2 * y1 - t1 * y2) * r,
            (t2 * z1 - t1 * z2) * r
        };

        XMFLOAT3 B{
            (s1 * x2 - s2 * x1) * r,
            (s1 * y2 - s2 * y1) * r,
            (s1 * z2 - s2 * z1) * r
        };

        for (Vertex* v : { &v0, &v1, &v2 }) {
            v->tx += T.x; v->ty += T.y; v->tz += T.z;
            v->bx += B.x; v->by += B.y; v->bz += B.z;
        }
    }

    for (auto& v : verts) {
        XMVECTOR n = XMVector3Normalize(XMLoadFloat3((XMFLOAT3*)&v.nx));
        XMVECTOR t = XMVector3Normalize(XMLoadFloat3((XMFLOAT3*)&v.tx));
        XMVECTOR b = XMVector3Normalize(XMLoadFloat3((XMFLOAT3*)&v.bx));

        XMStoreFloat3((XMFLOAT3*)&v.tx, t);
        XMStoreFloat3((XMFLOAT3*)&v.bx, b);
    }
}

static void LoadOBJIntoAsset(const std::string& filename, MeshAsset& out, std::shared_ptr<Texture> defaultWhite)
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
    std::unordered_map<std::string, std::shared_ptr<Texture>> materialNormalTextures;

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
    auto getMaterialNormal = [&](const std::string& name) -> std::shared_ptr<Texture>
        {
            auto it = materialNormalTextures.find(name);
            if (it != materialNormalTextures.end())
                return it->second;

            auto matIt = materials.find(name);
            if (matIt == materials.end() || matIt->second.map_normal.empty()) {
                materialNormalTextures[name] = nullptr;
                return nullptr;
            }

            const std::string texPath = joinPath(baseDir, matIt->second.map_normal);

            auto tex = std::make_shared<Texture>();
            try
            {
                auto& win = WindowDX12::Get();
                auto& gd = win.GetGraphicsDevice();
                auto  alloc = win.AllocateSrv();

                tex->LoadFromFile(gd, texPath.c_str(), alloc.cpu, alloc.gpu);

                materialNormalTextures[name] = tex;
                return tex;
            }
            catch (...)
            {
                std::cerr << "Error normal map: " << texPath << "\n";
                materialNormalTextures[name] = nullptr;
                return nullptr;
            }
        };

    auto beginSubmesh = [&](const std::string& name, const Material* mat)
        {
            Submesh sm;
            sm.indexStart = static_cast<uint32_t>(out.indices.size());
            if (mat) {
                sm.kd = mat->Kd;
                sm.shininess = mat->Ns;
                if (auto tex = getMaterialTexture(name))
                    sm.texture = tex;
                else
                    sm.texture = defaultWhite;
            }
            else {
                sm.kd = DirectX::XMFLOAT3(1.f, 1.f, 1.f);
                sm.shininess = 128.f;
                sm.texture = defaultWhite;
            }

            if (mat && !mat->map_normal.empty()) {
                if (auto n = getMaterialNormal(name)) {
                    sm.normalMap = n;
                    sm.hasNormalMap = true;
                }
            }

            out.submeshes.push_back(sm);
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

            if (out.submeshes.empty())
                beginSubmesh(currentMaterialName, currentMaterial);

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

    if (!out.submeshes.empty()) {
        auto& last = out.submeshes.back();
        last.indexCount = static_cast<uint32_t>(out.indices.size() - last.indexStart);
    }

    if (!out.texture)
        out.texture = defaultWhite;

    const bool hasNormals = !normals.empty();
    if (!hasNormals) {
		std::cout << "Recomputing smooth normals for " << filename << "\n";
        RecomputeSmoothNormals(out.vertices, out.indices);
    }


    ComputeTangents(out.vertices, out.indices);

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
