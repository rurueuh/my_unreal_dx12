#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>
#include "MeshAsset.h"

struct Material {
    DirectX::XMFLOAT3 Kd{ 1,1,1 };
    DirectX::XMFLOAT3 Ks{ 1,1,1 };
    DirectX::XMFLOAT3 Ke{ 0,0,0 };
    std::string map_Kd;
    float Ns{ 128.f };
    float d{ 1.f };
    std::string map_normal;
};

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
