#include "ResourceCache.h"
#include "Mesh.h"
#include "windowDX12.h"

static void LoadOBJIntoAsset(const std::string& filename, MeshAsset& out, std::shared_ptr<Texture> defaultWhite);

std::shared_ptr<MeshAsset> ResourceCache::getMeshFromOBJ(const std::string& path) {
    std::lock_guard<std::mutex> lk(mu_);
    if (auto sp = meshCache_[path].lock())
        return sp;

    auto asset = std::make_shared<MeshAsset>();
    LoadOBJIntoAsset(path, *asset, defaultWhite_);
    asset->Upload(WindowDX12::Get().GetDevice());
    meshCache_[path] = asset;
    return asset;
}