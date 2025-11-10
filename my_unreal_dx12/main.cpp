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

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	
    WindowDX12::ActivateConsole();
	auto& win = WindowDX12::Get();
	
	win.setWindowTitle(L"My ruru");
	win.setWindowSize(1280, 800);
	srand(static_cast<unsigned int>(time(nullptr)));

	std::vector<std::shared_ptr<Mesh>> weapons = {};
	auto t = std::make_shared<Mesh>("mirage2000/scene.obj");
	t->SetPosition(0.f, -10.f, 30.f);
	weapons.push_back(t);
	auto meshDraw = win.getImGui().addText("Mesh: 0");
	win.getImGui().AddButton("Add Fighter Jet", [&weapons, &win, &meshDraw]() {
		std::shared_ptr<Mesh> weapon = std::make_shared<Mesh>("mirage2000/scene.obj");
		weapon->SetPosition(((rand() % 100) / 100.f - 0.5f) * 50.f,
			((rand() % 100) / 100.f - 0.5f) * 50.f,
			((rand() % 100) / 100.f - 0.5f) * 50.f
		);
		weapons.push_back(std::move(weapon));
		meshDraw->setText("Mesh: %u", weapons.size());
	});

	win.getImGui().addSeparator();
	float rotateFighter = 0.0f;
	weapons[0]->SetRotationYawPitchRoll(3.0f, 0.0f, 0.0f);
	win.getImGui().addSliderFloat("rotate fighter0", &rotateFighter, 0.0f, 360.0f, [&weapons](float val) {
		if (weapons.size() > 0) {
			weapons[0]->SetRotationYawPitchRoll(0.0f, val, 0.0f);
		}
	});

	auto triangleText = win.getImGui().addText("Triangles: 0");


    while (win.IsOpen())
    {
        auto v = win.Clear();
        for (auto& c : weapons) {
            win.Draw(*c);
        }
		triangleText->setText("Triangles: %u", v);
        win.Display();
    }
}
