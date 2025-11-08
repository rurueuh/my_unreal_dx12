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
	std::vector<Vertex> m_vertices;
	std::vector<uint16_t> m_indices;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vb, m_ib;
	D3D12_VERTEX_BUFFER_VIEW m_vbv{};
	D3D12_INDEX_BUFFER_VIEW m_ibv{};
	UINT m_indexCount = 0;

	std::shared_ptr<Texture> m_texture;

	void Upload(ID3D12Device* device);
};

