#include "WindowDX12.h"

WindowDX12::WindowDX12(UINT w, UINT h, const std::wstring& title)
{
#ifdef _DEBUG
    Microsoft::WRL::ComPtr<ID3D12Debug1> dbg;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dbg)))) {
        dbg->EnableDebugLayer();
        Microsoft::WRL::ComPtr<ID3D12Debug1> dbg1;
        if (SUCCEEDED(dbg.As(&dbg1))) {
            dbg1->SetEnableGPUBasedValidation(TRUE);
        }
    }
#endif
    m_window.Create(title, w, h);
    m_gfx.Initialize();

    {
        D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
        srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvDesc.NumDescriptors = 1024;
        srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        srvDesc.NodeMask = 0;

        DXThrow(m_gfx.Device()->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&m_srvHeap)));
        m_srvDescriptorSize = m_gfx.Device()->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_nextSrvIndex = 0;
    }

    m_swap.Create(m_gfx, m_window.GetHwnd(), w, h);
    m_depth.Create(m_gfx, w, h);

    {
        auto shadowSrv = AllocateSrv();
        m_shadowMap.Initialize(m_gfx.Device(), 4096, 4096, shadowSrv.cpu, shadowSrv.gpu);
    }

    m_imgui.Init(m_window.GetHwnd(), m_gfx, m_swap);
    m_imgui.addText("DirectX 12 Renderer DEBUGGER");
    m_imgui.addFPS();
    m_imgui.AddButton("Toggle Wireframe", [this]() {
        static bool wireframe = false;
        wireframe = !wireframe;
        setWireframe(wireframe);
    });
    m_imgui.AddButton("Reload Shaders", [this]() {
        m_reloadShadersRequested = true;
    });
    m_imgui.addSeparator();
    m_imgui.addSliderFloat("Camera Speed",
        &m_camController.GetMoveSpeed(), 0.1f, 20.0f,
        [this](float val) {
            m_camController.SetMoveSpeeds(val, val * 5.f);
     });

    m_renderer.Initialize(m_gfx, m_swap, m_depth);

    CreateShader();

    {
        D3D12_INPUT_ELEMENT_DESC shadowIL[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        char* shadowVsSrc = nullptr;
        char* shadowPsSrc = nullptr;

        std::ifstream vsFile("ShadowVertex.hlsl", std::ios::in | std::ios::binary);
        std::ifstream psFile("ShadowPixel.hlsl", std::ios::in | std::ios::binary);
        if (!vsFile.is_open() || !psFile.is_open()) {
            throw std::runtime_error("Failed to open shadow shader files.");
        }

        vsFile.seekg(0, std::ios::end);
        size_t vsSize = static_cast<size_t>(vsFile.tellg());
        vsFile.seekg(0, std::ios::beg);
        shadowVsSrc = new char[vsSize + 1];
        vsFile.read(shadowVsSrc, vsSize);
        shadowVsSrc[vsSize] = '\0';
        vsFile.close();

        psFile.seekg(0, std::ios::end);
        size_t psSize = static_cast<size_t>(psFile.tellg());
        psFile.seekg(0, std::ios::beg);
        shadowPsSrc = new char[psSize + 1];
        psFile.read(shadowPsSrc, psSize);
        shadowPsSrc[psSize] = '\0';
        psFile.close();

        m_shadowPipeline.Create(
            m_gfx.Device(),
            shadowIL, _countof(shadowIL),
            shadowVsSrc, shadowPsSrc,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_D32_FLOAT);

        delete[] shadowVsSrc;
        delete[] shadowPsSrc;
    }

    m_cb.Create(m_gfx.Device(), kSwapBufferCount * kMaxDrawsPerFrame);

    const float aspect = float(m_window.GetWidth()) / float(m_window.GetHeight());
    m_camera.LookAt(
        DirectX::XMVectorSet(0, 0, -5, 0),
        DirectX::XMVectorSet(0, 0, 0, 0),
        DirectX::XMVectorSet(0, 1, 0, 0));
    m_camera.SetPerspective(
        DirectX::XM_PIDIV4, aspect,
        m_camController.getNearZ(), m_camController.getFarZ());
    m_view = m_camera.View();
    m_proj = m_camera.Proj();

    m_camController = CameraController();
    m_camController.SetPosition({ 0, 0, -5 });
    m_camController.SetYawPitch(0.0f, 0.0f);
    m_camController.SetProj(
        DirectX::XM_PIDIV4,
        m_camController.getNearZ(),
        m_camController.getFarZ());

    if (!m_whitePtr) {
        auto handles = AllocateSrv();
        m_whitePtr = std::make_shared<Texture>();
        m_whitePtr->InitWhite1x1(m_gfx, handles.cpu, handles.gpu);
    }
    ResourceCache::I().setDefaultWhiteTexture(getDefaultTextureShared());

#if _DEBUG
    {
        Microsoft::WRL::ComPtr<ID3D12InfoQueue> q;
        if (SUCCEEDED(GetDevice()->QueryInterface(IID_PPV_ARGS(&q)))) {
            q->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            q->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        }
    }
#endif
}

void WindowDX12::CreateShader(void)
{
    D3D12_INPUT_ELEMENT_DESC il[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 36,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    char* vertexShaderSrc = nullptr;
    char* pixelShaderSrc = nullptr;
    {
        std::ifstream vsFile("VertexShader.hlsl", std::ios::in | std::ios::binary);
        std::ifstream psFile("PixelShader.hlsl", std::ios::in | std::ios::binary);
        if (!vsFile.is_open() || !psFile.is_open()) {
            throw std::runtime_error("Failed to open shader files.");
        }

        vsFile.seekg(0, std::ios::end);
        size_t vsSize = static_cast<size_t>(vsFile.tellg());
        vsFile.seekg(0, std::ios::beg);
        vertexShaderSrc = new char[vsSize + 1];
        vsFile.read(vertexShaderSrc, vsSize);
        vertexShaderSrc[vsSize] = '\0';
        vsFile.close();

        psFile.seekg(0, std::ios::end);
        size_t psSize = static_cast<size_t>(psFile.tellg());
        psFile.seekg(0, std::ios::beg);
        pixelShaderSrc = new char[psSize + 1];
        psFile.read(pixelShaderSrc, psSize);
        pixelShaderSrc[psSize] = '\0';
        psFile.close();
    }

    m_pipeline.Create(
        m_gfx.Device(),
        il, _countof(il),
        vertexShaderSrc, pixelShaderSrc,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_D32_FLOAT);
    m_renderer.SetPipeline(m_pipeline);

    delete[] vertexShaderSrc;
    delete[] pixelShaderSrc;
}

SrvHandlePair WindowDX12::AllocateSrv()
{
    SrvHandlePair h{};
    auto cpuStart = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
    auto gpuStart = m_srvHeap->GetGPUDescriptorHandleForHeapStart();

    cpuStart.ptr += SIZE_T(m_nextSrvIndex) * m_srvDescriptorSize;
    gpuStart.ptr += SIZE_T(m_nextSrvIndex) * m_srvDescriptorSize;

    h.cpu = cpuStart;
    h.gpu = gpuStart;
    ++m_nextSrvIndex;
    return h;
}

uint32_t WindowDX12::Clear()
{
    if (m_reloadShadersRequested)
    {
        m_gfx.WaitGPU();
        m_pipeline.Destroy();
        CreateShader();
        m_reloadShadersRequested = false;
    }
    if (m_window.WasResized()) {
        m_gfx.WaitGPU();
        m_swap.Resize(m_gfx, m_window.GetWidth(), m_window.GetHeight());
        m_depth.Resize(m_gfx, m_window.GetWidth(), m_window.GetHeight());
        m_renderer.OnResize(m_window.GetWidth(), m_window.GetHeight());

        const float aspect = float(m_window.GetWidth()) / float(m_window.GetHeight());
        m_camera.SetPerspective(DirectX::XM_PIDIV4, aspect,
            m_camController.getNearZ(), m_camController.getFarZ());
        m_view = m_camera.View();
        m_proj = m_camera.Proj();
    }

    const auto now = std::chrono::steady_clock::now();
    dt = std::chrono::duration<float>(now - m_t0).count();
    m_t0 = now;
    m_camController.Update(dt, m_camera,
        float(m_window.GetWidth()) / float(m_window.GetHeight()));

    m_drawCursor = 0;

    using namespace DirectX;

    XMVECTOR lightDirRays = XMVector3Normalize(XMVectorSet(0.3f, -1.0f, 0.3f, 0.0f));
    XMVECTOR center = XMVectorSet(m_camera.getPosition().x, m_camera.getPosition().y, m_camera.getPosition().z, 10);
    XMVECTOR lightPos = XMVectorSubtract(center, XMVectorScale(lightDirRays, 80.0f));

    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, center, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    XMMATRIX lightProj = XMMatrixOrthographicLH(60.f, 60.f, 1.0f, 150.0f);
    XMStoreFloat4x4(&m_lightViewProj, XMMatrixTranspose(lightView * lightProj));

    XMFLOAT3 lightDirShader;
    XMStoreFloat3(&lightDirShader, XMVectorNegate(lightDirRays));
    m_lightDir = lightDirShader;

    m_renderer.BeginFrame(m_swap.FrameIndex());
    m_imgui.NewFrame();

    ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
    m_renderer.GetCommandList()->SetDescriptorHeaps(1, heaps);

    auto s = m_trianglesCount;
    m_trianglesCount = 0;
    m_DrawList.clear();
    return s;
}

void WindowDX12::RenderShadowPass(const std::vector<Mesh*>& meshes)
{
    using namespace DirectX;

    ID3D12GraphicsCommandList* cmd = m_renderer.GetCommandList();
    m_renderer.BeginShadowPass(m_shadowMap, m_shadowPipeline);

    const UINT frame = m_swap.FrameIndex();

    for (auto* mesh : meshes)
    {
        XMMATRIX M = mesh->Transform();

        SceneCB cb{};
        XMStoreFloat4x4(&cb.uModel, XMMatrixTranspose(M));
        cb.uLightViewProj = m_lightViewProj;

        UINT slice = frame * kMaxDrawsPerFrame + (m_drawCursor++);
        D3D12_GPU_VIRTUAL_ADDRESS addr = m_cb.UploadSlice(slice, cb);

        m_renderer.DrawMeshShadow(*mesh, addr);
    }

    m_renderer.EndShadowPass(m_shadowMap);
    m_renderer.BindMainRenderTargets();
}

void WindowDX12::Draw(const Mesh& mesh)
{
    m_DrawList.push_back(const_cast<Mesh*>(&mesh));
}

void WindowDX12::Display()
{
    RenderShadowPass(m_DrawList);
    DrawScene();
    m_imgui.Draw(m_renderer);
    const UINT frame = m_swap.FrameIndex();
    m_renderer.EndFrame(frame);
}

void WindowDX12::SetCameraLookAt(DirectX::XMVECTOR eye, DirectX::XMVECTOR at, DirectX::XMVECTOR up)
{
    m_camera.LookAt(eye, at, up);
    m_view = m_camera.View();
}

void WindowDX12::SetCameraPerspective(float fov, float aspect, float zn, float zf)
{
    m_camera.SetPerspective(fov, aspect, zn, zf);
    m_proj = m_camera.Proj();
}

void WindowDX12::ActivateConsole()
{
    AllocConsole();
    SetConsoleTitleW(L"Debug Output");
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
}

void WindowDX12::DrawScene() {
    using namespace DirectX;

    for (auto& meshPtr : m_DrawList) {
        XMMATRIX M = meshPtr->Transform();
        XMMATRIX V = m_camera.View();
        XMMATRIX P = m_camera.Proj();
        XMMATRIX VP = V * P;

        XMFLOAT3 camPos = m_camera.getPosition();

        XMVECTOR det;
        XMMATRIX MInv = XMMatrixInverse(&det, M);
        XMMATRIX NMat = XMMatrixTranspose(MInv);

        SceneCB base{};
        XMStoreFloat4x4(&base.uModel, XMMatrixTranspose(M));
        XMStoreFloat4x4(&base.uViewProj, XMMatrixTranspose(VP));
        XMStoreFloat4x4(&base.uNormalMatrix, XMMatrixTranspose(NMat));
        base.uCameraPos = camPos;
        base.uLightViewProj = m_lightViewProj;
        base.uLightDir = m_lightDir;
        base._pad0 = 0.0f;

        const UINT frame = m_swap.FrameIndex();

        const MeshAsset* asset = meshPtr->GetAsset();

        if (asset && !asset->submeshes.empty()) {
            for (const auto& sm : asset->submeshes) {
                SceneCB cb = base;
                cb.uShininess = sm.shininess;

                const UINT slice = frame * kMaxDrawsPerFrame + (m_drawCursor++);
                D3D12_GPU_VIRTUAL_ADDRESS addr = m_cb.UploadSlice(slice, cb);

                Texture* tex = nullptr;
                if (sm.texture)
                    tex = sm.texture.get();
                else
                    tex = meshPtr->GetTexture();
                if (!tex)
                    tex = &getDefaultTexture();

                D3D12_GPU_DESCRIPTOR_HANDLE texHandle = tex->GPUHandle();
                D3D12_GPU_DESCRIPTOR_HANDLE shadowHandle = m_shadowMap.SRVGPU();

                m_renderer.DrawMeshRange(*meshPtr, addr, texHandle, shadowHandle,
                    sm.indexStart, sm.indexCount);
                m_trianglesCount += sm.indexCount / 3;
            }
        }
        else {
            SceneCB cb = base;
            cb.uShininess = meshPtr->getShininess();

            const UINT slice = frame * kMaxDrawsPerFrame + (m_drawCursor++);
            D3D12_GPU_VIRTUAL_ADDRESS addr = m_cb.UploadSlice(slice, cb);

            auto* tex = meshPtr->GetTexture();
            if (!tex)
                tex = &getDefaultTexture();

            D3D12_GPU_DESCRIPTOR_HANDLE texHandle = tex->GPUHandle();
            D3D12_GPU_DESCRIPTOR_HANDLE shadowHandle = m_shadowMap.SRVGPU();

            m_renderer.DrawMesh(*meshPtr, addr, texHandle, shadowHandle);
            m_trianglesCount += meshPtr->IndexCount() / 3;
        }
    }
}
