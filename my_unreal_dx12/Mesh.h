#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <vector>
#include <cstdint>
#include <DirectXMath.h>
#include <string>
#include "Texture.h"
#include "Utils.h"

struct Vertex { float px, py, pz; float r, g, b; float u, v; };

class WindowsDX12;

class Mesh
{
public:
	Mesh() = default;
	Mesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices);

    Mesh objLoader(const std::string& filename);

	void Upload(ID3D12Device* device, const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices);

	void SetTexture(ID3D12Device* device, ID3D12DescriptorHeap* srvHeap, UINT heapIndex, ID3D12Resource* texture)
	{
		m_texture = texture;

		D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
		srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv.Format = m_texture->GetDesc().Format;
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = m_texture->GetDesc().MipLevels;

		auto cpuStart = srvHeap->GetCPUDescriptorHandleForHeapStart();
		auto inc = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ cpuStart.ptr + heapIndex * inc };
		device->CreateShaderResourceView(m_texture.Get(), &srv, cpuHandle);

		auto gpuStart = srvHeap->GetGPUDescriptorHandleForHeapStart();
		m_texGpuHandle = { gpuStart.ptr + heapIndex * inc };
	}

	void BindTexture(ID3D12GraphicsCommandList* cmdList, ID3D12DescriptorHeap* srvHeap, UINT rootParamIndex/*=table t0*/) {
		ID3D12DescriptorHeap* heaps[] = { srvHeap };
		cmdList->SetDescriptorHeaps(1, heaps);
		cmdList->SetGraphicsRootDescriptorTable(rootParamIndex, m_texGpuHandle);
	}

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


	const D3D12_VERTEX_BUFFER_VIEW& VBV() const { return m_vbv; }
	const D3D12_INDEX_BUFFER_VIEW& IBV() const { return m_ibv; }
	UINT IndexCount() const { return m_indexCount; }


	Texture m_tex;
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vb, m_ib;
	D3D12_VERTEX_BUFFER_VIEW m_vbv{};
	D3D12_INDEX_BUFFER_VIEW m_ibv{};
	DirectX::XMMATRIX m_transform{ DirectX::XMMatrixIdentity() };

	Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;
	D3D12_GPU_DESCRIPTOR_HANDLE m_texGpuHandle{ 0 };

	UINT m_indexCount = 0;
};