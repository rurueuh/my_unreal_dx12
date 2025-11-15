#pragma once
#include <DirectXMath.h>


class Camera
{
public:
	void SetPerspective(float fovRadians, float aspect, float zNear, float zFar)
	{
		using namespace DirectX;
		m_proj = XMMatrixPerspectiveFovLH(fovRadians, aspect, zNear, zFar);
	}


	void LookAt(DirectX::XMVECTOR eye, DirectX::XMVECTOR at, DirectX::XMVECTOR up)
	{
		using namespace DirectX;
		m_view = XMMatrixLookAtLH(eye, at, up);
	}


	DirectX::XMMATRIX View() const { return m_view; }
	DirectX::XMMATRIX Proj() const { return m_proj; }

	DirectX::XMFLOAT3 getPosition() const
	{
		using namespace DirectX;
		XMVECTOR det;
		XMMATRIX invView = XMMatrixInverse(&det, m_view);
		XMVECTOR pos = invView.r[3];
		XMFLOAT3 position;
		XMStoreFloat3(&position, pos);
		return position;
	}


private:
	DirectX::XMMATRIX m_view = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX m_proj = DirectX::XMMatrixIdentity();
};