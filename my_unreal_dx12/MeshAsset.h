#pragma once
#include <memory>
#include <vector>
#include <wrl.h>
#include <d3d12.h>
#include "Texture.h"

struct Vertex;

class MeshAsset
{
public:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	float shininess = 128.f;

	Microsoft::WRL::ComPtr<ID3D12Resource> vb, ib;
	D3D12_VERTEX_BUFFER_VIEW vbv{};
	D3D12_INDEX_BUFFER_VIEW ibv{};
	UINT indexCount = 0;

	std::shared_ptr<Texture> texture;

	void Upload(ID3D12Device* device);
};

