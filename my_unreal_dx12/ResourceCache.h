#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>
#include "MeshAsset.h"

class ResourceCache {
public:
    static ResourceCache& I() { static ResourceCache s; return s; }

    std::shared_ptr<MeshAsset> getMeshFromOBJ(const std::string& path);

    void setDefaultWhiteTexture(std::shared_ptr<Texture> t) {
        std::lock_guard<std::mutex> lk(mu_);
        defaultWhite_ = std::move(t);
    }
    std::shared_ptr<Texture> defaultWhite() {
        std::lock_guard<std::mutex> lk(mu_);
        return defaultWhite_;
    }

private:
    ResourceCache() = default;
    std::mutex mu_;
    std::unordered_map<std::string, std::weak_ptr<MeshAsset>> meshCache_;
    std::shared_ptr<Texture> defaultWhite_;
};
