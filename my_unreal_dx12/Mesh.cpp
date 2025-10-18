#include "Mesh.h"
#include "WindowDX12.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices) {
	this->Upload(WindowDX12::Get().GetDevice(), vertices, indices);
	std::cout << "Mesh created with " << indices.size() / 3 << " triangles.\n";
}

Mesh Mesh::objLoader(const std::string& filename)
{
    Mesh mesh;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erreur: impossible d’ouvrir " << filename << std::endl;
        return mesh;
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
            std::string vtx[3];
            iss >> vtx[0] >> vtx[1] >> vtx[2];
            for (int i = 0; i < 3; ++i) {
                if (vertexMap.find(vtx[i]) == vertexMap.end()) {
                    std::replace(vtx[i].begin(), vtx[i].end(), '/', ' ');
                    std::istringstream vss(vtx[i]);
                    int vi = 0, ti = 0, ni = 0;
                    vss >> vi >> ti >> ni;

                    Vertex vert{};
                    auto& p = positions[vi - 1];
                    vert.px = p.x;
                    vert.py = p.y;
                    vert.pz = p.z;

                    vert.r = fmod(std::abs(p.x), 1.0f);
                    vert.g = fmod(std::abs(p.y), 1.0f);
                    vert.b = fmod(std::abs(p.z), 1.0f);

					vert.r = 1.0f;
					vert.g = 1.0f;
					vert.b = 1.0f;

                    if (ti > 0) {
                        auto& t = texcoords[ti - 1];
                        vert.u = t.x;
                        vert.v = 1.0f - t.y;
					}

                    vertices.push_back(vert);
                    vertexMap[vtx[i]] = nextIndex++;
                }
                indices.push_back(vertexMap[vtx[i]]);
            }
        }
    }

    mesh.Upload(nullptr, vertices, indices);
    std::cout << "OBJ chargé: " << vertices.size() << " sommets, " << indices.size() / 3 << " faces." << std::endl;
    return mesh;
}

void Mesh::Upload(ID3D12Device* device, const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices)
{
	if (!device)
		device = WindowDX12::Get().GetDevice();
	D3D12_HEAP_PROPERTIES uploadHeap{}; uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
	D3D12_RESOURCE_DESC vbDesc{}; vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vbDesc.Width = UINT(vertices.size() * sizeof(Vertex));
	vbDesc.Height = 1;
	vbDesc.DepthOrArraySize = 1;
	vbDesc.MipLevels = 1;
	vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	vbDesc.SampleDesc = { 1,0 };

	DXThrow(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &vbDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vb)));
	{
		void* map = nullptr;
		D3D12_RANGE r{ 0,0 };
		m_vb->Map(0, &r, &map);
		memcpy(map, vertices.data(), vertices.size() * sizeof(Vertex));
		m_vb->Unmap(0, nullptr);
	}
	m_vbv.BufferLocation = m_vb->GetGPUVirtualAddress();
	m_vbv.StrideInBytes = sizeof(Vertex);
	m_vbv.SizeInBytes = UINT(vertices.size() * sizeof(Vertex));


	D3D12_RESOURCE_DESC ibDesc = vbDesc;
	ibDesc.Width = UINT(indices.size() * sizeof(uint16_t));
	DXThrow(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &ibDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_ib)));
	{
		void* map = nullptr;
		D3D12_RANGE r{ 0,0 };
		m_ib->Map(0, &r, &map);
		memcpy(map, indices.data(), indices.size() * sizeof(uint16_t));
		m_ib->Unmap(0, nullptr);
	}
	m_ibv.BufferLocation = m_ib->GetGPUVirtualAddress();
	m_ibv.Format = DXGI_FORMAT_R16_UINT;
	m_ibv.SizeInBytes = UINT(indices.size() * sizeof(uint16_t));


	m_indexCount = UINT(indices.size());
}
