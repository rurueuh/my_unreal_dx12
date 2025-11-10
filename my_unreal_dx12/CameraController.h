#pragma once
#include <windows.h>
#include <DirectXMath.h>
#include "Camera.h"
#include <iostream>

class CameraController {
public:
    CameraController() {};
	virtual ~CameraController() {};

    void SetPosition(DirectX::XMFLOAT3 pos) { m_pos = pos; }
    void SetYawPitch(float yaw, float pitch) { m_yaw = yaw; m_pitch = pitch; }

    void Update(float dt, Camera& cam, float aspect) {
        using namespace DirectX;

        float speed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? m_moveSpeedFast : m_moveSpeed;
        XMVECTOR forward, right, up = XMVectorSet(0, 1, 0, 0);
        XMVECTOR dir = YawPitchToDir(m_yaw, m_pitch);
        forward = XMVector3Normalize(dir);
        right = XMVector3Normalize(XMVector3Cross(up, forward));

        if (Key('Z')) m_pos = Add(m_pos, forward, speed * dt);
        if (Key('S')) m_pos = Add(m_pos, forward, -speed * dt);
        if (Key('Q')) m_pos = Add(m_pos, right, -speed * dt);
        if (Key('D')) m_pos = Add(m_pos, right, speed * dt);
        if (Key('E')) m_pos = Add(m_pos, up, speed * dt);
        if (Key('A')) m_pos = Add(m_pos, up, -speed * dt);

        if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
            POINT p; GetCursorPos(&p);
            if (!m_rotActive) {
                m_rotActive = true;
                m_last = p;
                ShowCursor(FALSE);
            }
            else {
                int dx = p.x - m_last.x;
                int dy = p.y - m_last.y;
                m_last = p;

                m_yaw += dx * m_sens;
                m_pitch -= dy * m_sens;
                constexpr float kPiOver2 = 1.553343f;
                if (m_pitch > kPiOver2) m_pitch = kPiOver2;
                if (m_pitch < -kPiOver2) m_pitch = -kPiOver2;
            }
        }
        else {
            if (m_rotActive) { m_rotActive = false; ShowCursor(TRUE); }
        }

        XMVECTOR eye = XMVectorSet(m_pos.x, m_pos.y, m_pos.z, 0);
        XMVECTOR at = XMVectorAdd(eye, YawPitchToDir(m_yaw, m_pitch));
        XMVECTOR upv = XMVectorSet(0, 1, 0, 0);

        cam.LookAt(eye, at, upv);
        cam.SetPerspective(m_fov, aspect, m_nearZ, m_farZ);
    }

    void SetMoveSpeeds(float normal, float fast) { m_moveSpeed = normal; m_moveSpeedFast = fast; }
	float &GetMoveSpeed() { return m_moveSpeed; }
    void SetMouseSensitivity(float sens) { m_sens = sens; }
	float GetMouseSensitivity() const { return m_sens; }
    void SetProj(float fov, float zn, float zf) { m_fov = fov; m_nearZ = zn; m_farZ = zf; }
	float GetFov() const { return m_fov; }
	float getNearZ() const { return m_nearZ; }
	float getFarZ() const { return m_farZ; }

private:
    bool Key(int vk) const { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

    static DirectX::XMVECTOR YawPitchToDir(float yaw, float pitch) {
        using namespace DirectX;
        float cy = cosf(yaw), sy = sinf(yaw);
        float cp = cosf(pitch), sp = sinf(pitch);
        return XMVectorSet(sy * cp, sp, cy * cp, 0);
    }

    static DirectX::XMFLOAT3 Add(DirectX::XMFLOAT3 p, DirectX::XMVECTOR v, float s) {
        using namespace DirectX;
        XMVECTOR pv = XMVectorSet(p.x, p.y, p.z, 0);
        pv = XMVectorAdd(pv, XMVectorScale(v, s));
        XMFLOAT3 out; XMStoreFloat3(&out, pv); return out;
    }

    POINT  m_last{};
    bool   m_rotActive = false;

    DirectX::XMFLOAT3 m_pos{ 0,0,-5 };
    float  m_yaw = 0.0f;           // radians
    float  m_pitch = 0.0f;         // radians

    float  m_moveSpeed = 3.0f;     // m/s
    float  m_moveSpeedFast = 9.0f;
    float  m_sens = 0.0025f;       // rad/pixel

    float  m_fov = DirectX::XM_PIDIV4;
    float  m_nearZ = 0.1f, m_farZ = 1000.0f;
};
