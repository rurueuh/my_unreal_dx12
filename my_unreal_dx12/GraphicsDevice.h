#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <iostream>
#include <stdexcept>
#include <cstdint>
#include <array>
#include "Utils.h"

using Microsoft::WRL::ComPtr;

class GraphicsDevice {
public:
    GraphicsDevice() = default;
    GraphicsDevice(const GraphicsDevice&) = delete;
    GraphicsDevice& operator=(const GraphicsDevice&) = delete;
    GraphicsDevice(GraphicsDevice&&) = default;
    GraphicsDevice& operator=(GraphicsDevice&&) = default;

    ~GraphicsDevice() noexcept {
        if (m_queue && m_fence) {
            try { WaitGPU(); }
            catch (...) {}
        }
    }

    void Initialize() {
#if defined(_DEBUG)
        if (ComPtr<ID3D12Debug> dbg; SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dbg)))) {
            dbg->EnableDebugLayer();
            if (ComPtr<ID3D12Debug1> dbg1; SUCCEEDED(dbg.As(&dbg1))) {
                dbg1->SetEnableGPUBasedValidation(TRUE);
                dbg1->SetEnableSynchronizedCommandQueueValidation(TRUE);
            }
        }
        UINT flags = DXGI_CREATE_FACTORY_DEBUG;
#else
        UINT flags = 0;
#endif
        DXThrow(CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory)));

        PickAdapterAndCreateDevice();

#if defined(_DEBUG)
        if (ComPtr<ID3D12InfoQueue> iq; SUCCEEDED(m_device.As(&iq))) {
            iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            D3D12_MESSAGE_ID hideIds[] = {
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
            };
            D3D12_INFO_QUEUE_FILTER f{};
            f.DenyList.NumIDs = (UINT)_countof(hideIds);
            f.DenyList.pIDList = hideIds;
            iq->AddStorageFilterEntries(&f);
        }
#endif

        D3D12_COMMAND_QUEUE_DESC q{};
        q.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        q.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        DXThrow(m_device->CreateCommandQueue(&q, IID_PPV_ARGS(&m_queue)));

        DXThrow(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue = 0;

        m_fenceEvent.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
        if (!m_fenceEvent) throw std::runtime_error("CreateEventEx failed");
    }

    IDXGIFactory7* Factory() const noexcept { return m_factory.Get(); }
    ID3D12Device* Device()  const noexcept { return m_device.Get(); }
    ID3D12CommandQueue* Queue()   const noexcept { return m_queue.Get(); }

    void WaitGPU() {
        const UINT64 v = ++m_fenceValue;
        DXThrow(m_queue->Signal(m_fence.Get(), v));
        if (m_fence->GetCompletedValue() < v) {
            DXThrow(m_fence->SetEventOnCompletion(v, m_fenceEvent.get()));
            const DWORD r = WaitForSingleObject(m_fenceEvent.get(), INFINITE);
            if (r == WAIT_FAILED) throw std::runtime_error("WaitForSingleObject failed");
        }
    }

private:
    struct unique_handle {
        unique_handle() = default;
        explicit unique_handle(HANDLE h) : h_(h) {}
        ~unique_handle() { reset(); }
        unique_handle(const unique_handle&) = delete;
        unique_handle& operator=(const unique_handle&) = delete;
        unique_handle(unique_handle&& o) noexcept : h_(o.h_) { o.h_ = nullptr; }
        unique_handle& operator=(unique_handle&& o) noexcept { if (this != &o) { reset(); h_ = o.h_; o.h_ = nullptr; } return *this; }
        void reset(HANDLE nh = nullptr) noexcept { if (h_) { CloseHandle(h_); } h_ = nh; }
        HANDLE get() const noexcept { return h_; }
        explicit operator bool() const noexcept { return h_ != nullptr; }
    private: HANDLE h_ = nullptr;
    };

    void PickAdapterAndCreateDevice() {
        ComPtr<IDXGIAdapter4> chosen;
        if (ComPtr<IDXGIFactory6> f6; SUCCEEDED(m_factory.As(&f6))) {
            for (UINT i = 0;; ++i) {
                ComPtr<IDXGIAdapter1> a1;
                if (f6->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                    IID_PPV_ARGS(&a1)) == DXGI_ERROR_NOT_FOUND) break;
                DXGI_ADAPTER_DESC1 d{}; a1->GetDesc1(&d);
                if (d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

                if (SUCCEEDED(a1.As(&chosen))) {
                    if (TryCreateDevice(chosen.Get())) return;
                }
            }
        }

        for (UINT i = 0;; ++i) {
            ComPtr<IDXGIAdapter1> a1;
            if (m_factory->EnumAdapters1(i, &a1) == DXGI_ERROR_NOT_FOUND) break;
            DXGI_ADAPTER_DESC1 d{}; a1->GetDesc1(&d);
            if (d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

            ComPtr<IDXGIAdapter4> a4;
            if (SUCCEEDED(a1.As(&a4)) && TryCreateDevice(a4.Get())) return;
        }

        ComPtr<IDXGIAdapter> warp;
        DXThrow(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
        DXThrow(D3D12CreateDevice(warp.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
    }

    bool TryCreateDevice(IDXGIAdapter4* adapter) {
        static constexpr std::array<D3D_FEATURE_LEVEL, 5> levels{
            D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0
        };
        for (auto fl : levels) {
            if (SUCCEEDED(D3D12CreateDevice(adapter, fl, IID_PPV_ARGS(&m_device)))) {
                DXGI_ADAPTER_DESC1 desc{};
                if (ComPtr<IDXGIAdapter1> a1; SUCCEEDED(adapter->QueryInterface(IID_PPV_ARGS(&a1)))) {
                    a1->GetDesc1(&desc);
                    std::wcout << L"[DX12] Using adapter: " << desc.Description << L" (FL "
                        << ((fl >> 12) & 0xF) << L"." << ((fl >> 8) & 0xF) << L")\n";
                }
                return true;
            }
        }
        return false;
    }

private:
    ComPtr<IDXGIFactory7>   m_factory;
    ComPtr<ID3D12Device>    m_device;
    ComPtr<ID3D12CommandQueue> m_queue;
    ComPtr<ID3D12Fence>     m_fence;
    UINT64                  m_fenceValue = 0;
    unique_handle           m_fenceEvent;
};
