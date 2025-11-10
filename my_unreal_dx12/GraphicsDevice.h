#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include "Utils.h"
#include <iostream>
#include <stdexcept>

using Microsoft::WRL::ComPtr;


class GraphicsDevice
{
public:
        ~GraphicsDevice()
        {
                if (m_fenceEvent)
                {
                        CloseHandle(m_fenceEvent);
                        m_fenceEvent = nullptr;
                }
        }

        void Initialize()
        {
#if defined(_DEBUG)
                if (ComPtr<ID3D12Debug> dbg; SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dbg))))
			dbg->EnableDebugLayer();
		UINT flags = DXGI_CREATE_FACTORY_DEBUG;
#else
		UINT flags = 0;
#endif
		DXThrow(CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory)));


		// Pick hardware adapter or fallback to WARP.
		ComPtr<IDXGIAdapter1> adapter;
		for (UINT i = 0; m_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 d{}; adapter->GetDesc1(&d);
			std::wcout << L"Found adapter: " << d.Description << L"\n";
			if (d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
			{
				DXThrow(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));
				break;
			}
		}
		if (!m_device)
		{
			ComPtr<IDXGIAdapter> warp; DXThrow(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
			DXThrow(D3D12CreateDevice(warp.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));
		}


		D3D12_COMMAND_QUEUE_DESC q{}; q.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		DXThrow(m_device->CreateCommandQueue(&q, IID_PPV_ARGS(&m_queue)));


                DXThrow(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
                m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
                if (!m_fenceEvent)
                {
                        throw std::runtime_error("Failed to create fence event");
                }
        }


	IDXGIFactory7* Factory() const { return m_factory.Get(); }
	ID3D12Device* Device() const { return m_device.Get(); }
	ID3D12CommandQueue* Queue() const { return m_queue.Get(); }


	void WaitGPU()
	{
		const UINT64 v = ++m_fenceValue;
		DXThrow(m_queue->Signal(m_fence.Get(), v));
		if (m_fence->GetCompletedValue() < v)
		{
			DXThrow(m_fence->SetEventOnCompletion(v, m_fenceEvent));
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}
	}


private:
	ComPtr<IDXGIFactory7> m_factory;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_queue;
	ComPtr<ID3D12Fence> m_fence; UINT64 m_fenceValue = 0; HANDLE m_fenceEvent = nullptr;
};