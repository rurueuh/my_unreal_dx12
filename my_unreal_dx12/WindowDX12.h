#pragma once
#include "GraphicsDevice.h"
#include "SwapChain.h"
#include "DepthBuffer.h"
#include "CommandContext.h"
#include "ShaderPipeline.h"
#include "Renderer.h"
#include "Shaders.h"
#include "Camera.h"
#include "Window.h"
#include "CameraController.h"
#include <chrono>
#include <wrl.h>
#include "ImGuiDx12.h"

class WindowDX12 {
public:
    WindowDX12(UINT w, UINT h, const std::wstring& title)
    {
        #ifdef _DEBUG
        ComPtr<ID3D12Debug1> dbg;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dbg)))) {
            dbg->EnableDebugLayer();
            ComPtr<ID3D12Debug1> dbg1;
            if (SUCCEEDED(dbg.As(&dbg1))) dbg1->SetEnableGPUBasedValidation(TRUE);
        }
        #endif
        m_window.Create(title, w, h);
        m_gfx.Initialize();
        m_swap.Create(m_gfx, m_window.GetHwnd(), w, h);
        m_depth.Create(m_gfx, w, h);
        
        m_imgui.Init(m_window.GetHwnd(), m_gfx, m_swap);
        m_imgui.addText("DirectX 12 Renderer DEBUGGER");
		m_imgui.addFPS();
        m_imgui.AddButton("Toggle Wireframe", [this]() {
            static bool wireframe = false;
            wireframe = !wireframe;
            setWireframe(wireframe);
		});
		m_imgui.addSeparator();
        m_imgui.addSliderFloat("Camera Speed", &m_camController.GetMoveSpeed(), 0.1f, 20.0f, [this](float val) {
            m_camController.SetMoveSpeeds(val, val * 5.f);
		});

        m_renderer.Initialize(m_gfx, m_swap, m_depth);

        D3D12_INPUT_ELEMENT_DESC il[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };
        m_pipeline.Create(m_gfx.Device(), il, _countof(il), kVertexShaderSrc, kPixelShaderSrc,
            DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT);
        m_renderer.SetPipeline(m_pipeline);

        m_cb.Create(m_gfx.Device(), kSwapBufferCount * kMaxDrawsPerFrame);

        const float aspect = float(m_window.GetWidth()) / float(m_window.GetHeight());
        m_camera.LookAt(DirectX::XMVectorSet(0, 0, -5, 0),
            DirectX::XMVectorSet(0, 0, 0, 0),
            DirectX::XMVectorSet(0, 1, 0, 0));
		m_camera.SetPerspective(DirectX::XM_PIDIV4, aspect, m_camController.getNearZ(), m_camController.getFarZ());
        m_view = m_camera.View();
        m_proj = m_camera.Proj();

        m_camController = CameraController();
        m_camController.SetPosition({ 0,0,-5 });
        m_camController.SetYawPitch(0.0f, 0.0f);
		m_camController.SetProj(DirectX::XM_PIDIV4, m_camController.getNearZ(), m_camController.getFarZ());
        if (!m_whitePtr) {
            m_whitePtr = std::make_shared<Texture>();
            m_whitePtr->InitWhite1x1(m_gfx);
        }
        ResourceCache::I().setDefaultWhiteTexture(getDefaultTextureShared());
#if _DEBUG
        ComPtr<ID3D12InfoQueue> q;
        GetDevice()->QueryInterface(IID_PPV_ARGS(&q));
        q->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        q->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
#endif
    }

    static WindowDX12& Get() {
        static WindowDX12 instance(800, 600, L"DX12 Window");
        return instance;
    }
    
    Texture &getDefaultTexture(void) {
		return *m_whitePtr;
	}
    std::shared_ptr<Texture> getDefaultTextureShared() {
        return m_whitePtr;
    }
    std::shared_ptr<const Texture> getDefaultTextureShared() const {
        return m_whitePtr;
    }

    ImGuiDx12& getImGui() {
        return m_imgui;
	}

    static void setWindowTitle(const std::wstring& title) {
        Get().m_window.SetTitle(title);
    }

    static void setWindowSize(UINT w, UINT h) {
        Get().m_window.SetSize(w, h);
	}

    void setWireframe(bool enable) {
        m_pipeline.setWireframe(enable);
    }

    bool IsOpen() { return m_window.PumpMessages(); }

    uint32_t Clear()
    {
        if (m_window.WasResized()) {
            m_gfx.WaitGPU();
            m_swap.Resize(m_gfx, m_window.GetWidth(), m_window.GetHeight());
            m_depth.Resize(m_gfx, m_window.GetWidth(), m_window.GetHeight());
            m_renderer.OnResize(m_window.GetWidth(), m_window.GetHeight());

            const float aspect = float(m_window.GetWidth()) / float(m_window.GetHeight());
			m_camera.SetPerspective(DirectX::XM_PIDIV4, aspect, m_camController.getNearZ(), m_camController.getFarZ());
            m_view = m_camera.View();
            m_proj = m_camera.Proj();
        }

        const auto now = std::chrono::steady_clock::now();
        dt = std::chrono::duration<float>(now - m_t0).count();
        m_t0 = now;
        m_camController.Update(dt, m_camera, float(m_window.GetWidth()) / float(m_window.GetHeight()));

        m_drawCursor = 0;
        m_renderer.BeginFrame(m_swap.FrameIndex());
		m_imgui.NewFrame();
        auto s = m_trianglesCount;
        m_trianglesCount = 0;
		return s;
    }

    void Draw(const Mesh& mesh)
    {
        using namespace DirectX;

        const XMMATRIX M = mesh.Transform();
        const XMMATRIX V = m_camera.View();
        const XMMATRIX P = m_camera.Proj();

        XMFLOAT4X4 mvp;
        XMStoreFloat4x4(&mvp, XMMatrixTranspose(M * V * P));

        const UINT frame = m_swap.FrameIndex();
        const UINT slice = frame * kMaxDrawsPerFrame + (m_drawCursor++);

        auto addr = m_cb.UploadSlice(slice, mvp);

        m_renderer.DrawMesh(mesh, addr);
		m_trianglesCount += mesh.IndexCount() / 3;
    }

    void Display()
    {
		m_imgui.Draw(m_renderer);
        const UINT frame = m_swap.FrameIndex();
        m_renderer.EndFrame(frame);
    }

    void SetCameraLookAt(DirectX::XMVECTOR eye, DirectX::XMVECTOR at, DirectX::XMVECTOR up) {
        m_camera.LookAt(eye, at, up);
        m_view = m_camera.View();
    }
    void SetCameraPerspective(float fov, float aspect, float zn, float zf) {
        m_camera.SetPerspective(fov, aspect, zn, zf);
        m_proj = m_camera.Proj();
    }

    ID3D12Device* GetDevice() const { return m_gfx.Device(); }
    GraphicsDevice& GetGraphicsDevice() { return m_gfx; }

    static void ActivateConsole()
    {
        AllocConsole();
        SetConsoleTitleW(L"Debug Output");
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
    }

private:
    inline static std::shared_ptr<Texture> m_whitePtr;
    static constexpr UINT kMaxDrawsPerFrame = 19000;

    Window          m_window;
    GraphicsDevice  m_gfx;
    SwapChain       m_swap;
    DepthBuffer     m_depth;
    Renderer        m_renderer;
    ShaderPipeline  m_pipeline;

    ConstantBuffer  m_cb{};
    UINT            m_drawCursor = 0;

    Camera          m_camera;
    CameraController m_camController;

	ImGuiDx12 m_imgui;

    DirectX::XMMATRIX m_view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX m_proj = DirectX::XMMatrixIdentity();

    std::chrono::steady_clock::time_point m_t0 = std::chrono::steady_clock::now();
    float dt = 0.0f;
	mutable uint32_t m_trianglesCount = 0;
};
