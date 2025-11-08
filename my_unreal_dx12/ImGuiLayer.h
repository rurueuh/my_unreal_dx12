#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "GraphicsDevice.h"
#include "SwapChain.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

class ImGuiLayer {
public:
    void Init(HWND hwnd, GraphicsDevice& gd, SwapChain& sc)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        {
            auto& io = ImGui::GetIO();
            io.Fonts->AddFontDefault();
            unsigned char* pixels = nullptr; int w = 0, h = 0;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
        }

        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        DXThrow(gd.Device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap)));

        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX12_Init(
            gd.Device(),
            sc.BufferCount(),
            DXGI_FORMAT_R8G8B8A8_UNORM,
            m_heap.Get(),
            m_heap->GetCPUDescriptorHandleForHeapStart(),
            m_heap->GetGPUDescriptorHandleForHeapStart()
        );
    }



    void NewFrame() {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void Render(ID3D12GraphicsCommandList* cmd) {
        ImGui::Render();
        ID3D12DescriptorHeap* heaps[] = { m_heap.Get() };
        cmd->SetDescriptorHeaps(1, heaps);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd);
    }

    void Shutdown() {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        m_heap.Reset();
    }

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap;
};
