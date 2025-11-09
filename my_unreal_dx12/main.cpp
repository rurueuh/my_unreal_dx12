#include <DirectXMath.h>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <tuple>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <algorithm>

#include "Utils.h"
#include "Window.h"
#include "GraphicsDevice.h"
#include "SwapChain.h"
#include "DepthBuffer.h"
#include "CommandContext.h"
#include "ShaderPipeline.h"
#include "ConstantBuffer.h"
#include "Mesh.h"
#include "Camera.h"
#include "CameraController.h"
#include "Renderer.h"
#include "Shaders.h"
#include "WindowDX12.h"
#include "main.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

static void getFps()
{
	auto& win = WindowDX12::Get();
	static std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
	static int farmeCount = 0;
	farmeCount++;
	if (farmeCount % 60 == 0) {
		auto currentTime = std::chrono::steady_clock::now();
		auto frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime).count();
		lastTime = currentTime;
		std::wcout << L"Frame time: " << frameTime << L" ms\r";
		float fps = 60000.0f / frameTime;
		std::wstring title = L"My ruru - FPS: " + std::to_wstring(fps);
		win.setWindowTitle(title);
	}
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    WindowDX12::ActivateConsole();
	auto& win = WindowDX12::Get();
	/*ComPtr<ID3D12Debug1> dbg;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dbg)))) {
		dbg->EnableDebugLayer();
		dbg->SetEnableGPUBasedValidation(TRUE);
	}*/
	ComPtr<ID3D12InfoQueue> q;
	win.GetDevice()->QueryInterface(IID_PPV_ARGS(&q));
	q->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
	q->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	win.setWindowTitle(L"My ruru");
	win.setWindowSize(1920, 1080);
	srand(static_cast<unsigned int>(time(nullptr)));

	std::vector<std::shared_ptr<Mesh>> weapons = {};
	auto t = std::make_shared<Mesh>("jet/fighter_jet.obj");
	t->SetPosition(0.f, -10.f, 30.f);
	t->SetScale(0.1f, 0.1f, 0.1f);
	weapons.push_back(t);
	auto meshDraw = win.getImGui().addText("Mesh: 0");
	win.getImGui().AddButton("Add Fighter Jet", [&weapons, &win, &meshDraw]() {
		std::shared_ptr<Mesh> weapon = std::make_shared<Mesh>("jet/fighter_jet.obj");
		weapon->SetPosition(((rand() % 100) / 100.f - 0.5f) * 50.f,
			((rand() % 100) / 100.f - 0.5f) * 50.f,
			((rand() % 100) / 100.f - 0.5f) * 50.f
		);
		weapon->SetScale(0.1f, 0.1f, 0.1f);
		weapons.push_back(std::move(weapon));
		meshDraw->setText("Mesh: %u", weapons.size());
	});

	auto triangleText = win.getImGui().addText("Triangles: 0");


    while (win.IsOpen())
    {
        auto v = win.Clear();
        for (auto& c : weapons) {
            win.Draw(*c);
        }
		triangleText->setText("Triangles: %u", v);
		getFps();
        win.Display();
    }
}
