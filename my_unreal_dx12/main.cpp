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
	win.setWindowSize(1920, 1080);
	srand(static_cast<unsigned int>(time(nullptr)));

	Mesh floor = Mesh::CreatePlane(100.0f, 100.0f, 10, 10);
	floor.SetPosition(0.f, -5.f, 0.f);
	floor.SetRotationYawPitchRoll(0.f, 0.f, 180.f);

	std::vector<std::shared_ptr<Mesh>> geometricsMeshes = {};
	auto cubeMesh = std::make_shared<Mesh>(Mesh::CreateCube(2.0f));
	cubeMesh->SetPosition(10.f, 0.f, 0.f);
	geometricsMeshes.push_back(cubeMesh);

	auto sphereMesh = std::make_shared<Mesh>(Mesh::CreateSphere(1.0f, 16, 16));
	sphereMesh->SetPosition(-10.f, 0.f, 0.f);
	geometricsMeshes.push_back(sphereMesh);

	auto cylinderMesh = std::make_shared<Mesh>(Mesh::CreateCylinder(1.0f, 2.0f, 16));
	cylinderMesh->SetPosition(0.f, 0.f, 10.f);
	geometricsMeshes.push_back(cylinderMesh);

	auto coneMesh = std::make_shared<Mesh>(Mesh::CreateCone(1.0f, 2.0f, 1600, true));
	coneMesh->SetPosition(0.f, 0.f, -10.f);
	geometricsMeshes.push_back(coneMesh);


	std::vector<std::shared_ptr<Mesh>> weapons = {};
	auto t = std::make_shared<Mesh>("mirage2000/scene.obj");
	t->SetPosition(0.f, 0.f, 0.f);
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

	win.getImGui().addSeparator();
	float rotateFighter = 0.0f;
	weapons[0]->SetRotationYawPitchRoll(3.0f, 0.0f, 0.0f);
	win.getImGui().addSliderFloat("rotate fighter0", &rotateFighter, 0.0f, 360.0f, [&weapons, &floor, &geometricsMeshes](float val) {
		if (weapons.size() > 0) {
			weapons[0]->SetRotationYawPitchRoll(0.0f, val, 0.0f);
			for (auto &g : geometricsMeshes) {
				g->SetRotationYawPitchRoll(0.0f, val, 0.0f);
			}
		}
	});

	win.getImGui().addSeparator();

	win.getImGui().addSliderFloat("translate fighter0 x", nullptr, -100.0f, 100.0f, [&weapons](float val) {
		if (weapons.size() > 0) {
			weapons[0]->SetPosition(0.0f, val, 0.0f);
			
		}
		});
	win.getImGui().addSliderFloat("translate fighter0 y", nullptr, -100.0f, 100.0f, [&weapons](float val) {
		if (weapons.size() > 0) {
			weapons[0]->SetPosition(0.0f, 0.0f, val);
			
		}
		});
	win.getImGui().addSliderFloat("translate fighter0 z", nullptr, -100.0f, 100.0f, [&weapons](float val) {
		if (weapons.size() > 0) {
			weapons[0]->SetPosition(val, 0.0f, 0.0f);
			
		}
	});



	auto triangleText = win.getImGui().addText("Triangles: 0");


    while (win.IsOpen())
    {
        auto v = win.Clear();
        for (auto& c : weapons) {
            //win.Draw(*c);
        }
		win.Draw(floor);
		for (auto& g : geometricsMeshes) {
			win.Draw(*g);
		}
		triangleText->setText("Triangles: %u", v);
        win.Display();
    }
}
