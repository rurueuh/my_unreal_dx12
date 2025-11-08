#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <vector>
#include <cstdint>
#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include "Texture.h"
#include "Utils.h"

struct Vertex { float px, py, pz; float r, g, b; float u, v; };
struct Material {
	DirectX::XMFLOAT3 Kd{ 1,1,1 };
	std::string map_Kd;
};

class WindowsDX12;

class Mesh
{
public:
	Mesh() = delete;
	Mesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices);
	Mesh(const std::string& filename);

	void Upload(ID3D12Device* device, const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices, ID3D12Fence* fence = nullptr, UINT64 fenceValue = 0);

	void SetTexture(Texture* tex) { m_texture = tex; }

	Texture* GetTexture() const { return m_texture ? m_texture : m_defaultWhite; }

	void SetDefaultTexture(Texture* texWhite) { m_defaultWhite = texWhite; }

	void BindTexture(ID3D12GraphicsCommandList* cmdList, UINT rootParamIndex) const;

	void SetPosition(float x, float y, float z)
	{
		m_transform.r[3] = DirectX::XMVectorSet(x, y, z, 1.0f);
	}
	void AddPosition(float dx, float dy, float dz)
	{
		DirectX::XMVECTOR pos = m_transform.r[3];
		pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorSet(dx, dy, dz, 0.0f));
		m_transform.r[3] = pos;
	}
	void SetRotationYawPitchRoll(float yaw, float pitch, float roll)
	{
		DirectX::XMMATRIX rot = DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
		m_transform.r[0] = rot.r[0];
		m_transform.r[1] = rot.r[1];
		m_transform.r[2] = rot.r[2];
	}
	void AddRotationYawPitchRoll(float dyaw, float dpitch, float droll)
	{
		DirectX::XMMATRIX currentRot;
		currentRot.r[0] = m_transform.r[0];
		currentRot.r[1] = m_transform.r[1];
		currentRot.r[2] = m_transform.r[2];
		currentRot.r[3] = DirectX::XMVectorSet(0, 0, 0, 1.0f);
		DirectX::XMMATRIX deltaRot = DirectX::XMMatrixRotationRollPitchYaw(dpitch, dyaw, droll);
		DirectX::XMMATRIX newRot = DirectX::XMMatrixMultiply(deltaRot, currentRot);
		m_transform.r[0] = newRot.r[0];
		m_transform.r[1] = newRot.r[1];
		m_transform.r[2] = newRot.r[2];
	}
	void SetScale(float sx, float sy, float sz)
	{
		m_transform.r[0] = DirectX::XMVectorSetX(m_transform.r[0], sx);
		m_transform.r[1] = DirectX::XMVectorSetY(m_transform.r[1], sy);
		m_transform.r[2] = DirectX::XMVectorSetZ(m_transform.r[2], sz);
	}
	void AddScale(float dsx, float dsy, float dsz)
	{
		m_transform.r[0] = DirectX::XMVectorAdd(m_transform.r[0], DirectX::XMVectorSet(dsx, 0, 0, 0));
		m_transform.r[1] = DirectX::XMVectorAdd(m_transform.r[1], DirectX::XMVectorSet(0, dsy, 0, 0));
		m_transform.r[2] = DirectX::XMVectorAdd(m_transform.r[2], DirectX::XMVectorSet(0, 0, dsz, 0));
	}
	const DirectX::XMMATRIX& Transform() const { return m_transform; }
	void setColor(float r, float g, float b)
	{
		for (auto& v : m_vertices)
		{
			v.r = r;
			v.g = g;
			v.b = b;
		}
		this->Upload(nullptr, m_vertices, m_indices);
	}
	std::tuple<float, float, float> getColor() const
	{
		Vertex v{};
		D3D12_RANGE readRange{ 0, sizeof(Vertex) };
		m_vb->Map(0, &readRange, reinterpret_cast<void**>(&v));
		m_vb->Unmap(0, nullptr);
		return { v.r, v.g, v.b };
	}


	const D3D12_VERTEX_BUFFER_VIEW& VBV() const { return m_vbv; }
	const D3D12_INDEX_BUFFER_VIEW& IBV() const { return m_ibv; }
	UINT IndexCount() const { return m_indexCount; }


	Microsoft::WRL::ComPtr<ID3D12Resource> m_vb, m_ib;
private:
	D3D12_VERTEX_BUFFER_VIEW m_vbv{};
	D3D12_INDEX_BUFFER_VIEW m_ibv{};
	DirectX::XMMATRIX m_transform{ DirectX::XMMatrixIdentity() };

	Texture* m_texture = nullptr;
	mutable Texture* m_defaultWhite = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE m_texGpuHandle{ 0 };

	std::vector<Vertex> m_vertices;
	std::vector<uint16_t> m_indices;
	
	UINT m_indexCount = 0;
	std::unordered_map<std::string, Material> m_materials;
	Texture m_localTexture;
	bool m_hasLocalTexture = false;
	std::wstring m_baseDirW;
};
