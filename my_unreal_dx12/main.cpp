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
#include <windows.h>

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
    srand(static_cast<unsigned int>(time(nullptr)));

    Mesh floor = Mesh::CreatePlane(100.0f, 100.0f, 2, 2);
    floor.SetPosition(0.f, -5.f, 0.f);
    floor.SetColor(0.3f, 0.0f, 0.1f);

    std::vector<std::shared_ptr<Mesh>> geometricsMeshes;

    auto cubeMesh = std::make_shared<Mesh>(Mesh::CreateCube(2.0f));
    cubeMesh->SetPosition(10.f, 0.f, 0.f);
    geometricsMeshes.push_back(cubeMesh);

    auto cubeMesh2 = std::make_shared<Mesh>(Mesh::CreateCube(2.0f));
    cubeMesh2->SetPosition(12.f, 0.f, 0.f);
    geometricsMeshes.push_back(cubeMesh2);

    auto sphereMesh = std::make_shared<Mesh>(Mesh::CreateSphere(1.0f, 16, 16));
    sphereMesh->SetPosition(-10.f, 0.f, 0.f);
    geometricsMeshes.push_back(sphereMesh);

    auto cylinderMesh = std::make_shared<Mesh>(Mesh::CreateCylinder(1.0f, 2.0f, 16));
    cylinderMesh->SetPosition(0.f, 0.f, 10.f);
    geometricsMeshes.push_back(cylinderMesh);

    auto coneMesh = std::make_shared<Mesh>(Mesh::CreateCone(1.0f, 2.0f, 1600, true));
    coneMesh->SetPosition(0.f, 0.f, -10.f);
    geometricsMeshes.push_back(coneMesh);

    std::vector<std::shared_ptr<Mesh>> weapons;

    auto meshDraw = win.getImGui().addText("Mesh: 0");
    win.getImGui().AddButton("Add 1 Test wall", [&weapons, &win, meshDraw]() {
            std::shared_ptr<Mesh> weapon = std::make_shared<Mesh>("test/brick_wall.obj");
            weapon->SetPosition(
                ((rand() % 100) / 100.f - 0.5f) * 50.f,
                ((rand() % 100) / 100.f - 0.5f) * 50.f,
                ((rand() % 100) / 100.f - 0.5f) * 50.f
            );
            weapons.push_back(std::move(weapon));
            meshDraw->setText("Mesh: %u", (unsigned)weapons.size());
        }
    );
    win.getImGui().AddButton("Add 1 Fighters Jets", [&weapons, &win, meshDraw]() {
        for (int lh = 1; lh--;) {
            std::shared_ptr<Mesh> weapon = std::make_shared<Mesh>("mirage2000/scene.obj");
            weapon->SetPosition(
                ((rand() % 100) / 100.f - 0.5f) * 50.f,
                ((rand() % 100) / 100.f - 0.5f) * 50.f,
                ((rand() % 100) / 100.f - 0.5f) * 50.f
            );
            weapons.push_back(std::move(weapon));
            meshDraw->setText("Mesh: %u", (unsigned)weapons.size());
        }
    });

    win.getImGui().addSeparator();

    float rotateFighter = 0.0f;
    win.getImGui().addSliderFloat(
        "rotate fighter0", &rotateFighter, 0.0f, 360.0f,
        [&weapons, &geometricsMeshes](float val)
        {
            if (!weapons.empty()) {
                weapons[0]->SetRotationYawPitchRoll(0.0f, val, 0.0f);
                for (auto& g : geometricsMeshes) {
                    g->SetRotationYawPitchRoll(0.0f, val, 0.0f);
                }
            }
        }
    );

    win.getImGui().addSeparator();

    auto triangleText = win.getImGui().addText("Triangles: 0");

    std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
    auto msFrame = win.getImGui().addText("Frame Time: 0 ms");

    win.getImGui().addSeparator();
    win.getImGui().addSliderFloat(
        "Plane Specular",
        nullptr,
        1.0f, 256.0f,
        [&weapons, &geometricsMeshes](float val)
        {
            for (auto& w : weapons) {
                w->setShininess(val);
            }
            for (auto& g : geometricsMeshes) {
                g->setShininess(val);
            }
        }
    );

    while (win.IsOpen())
    {
        uint32_t trianglesLastFrame = win.Clear();

        for (auto& w : weapons)
            win.Draw(*w);

        win.Draw(floor);

        for (auto& g : geometricsMeshes)
            win.Draw(*g);

        auto currentTime = std::chrono::steady_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime).count();
        lastTime = currentTime;

        msFrame->setText("Frame Time: %lld ms", frameDuration);
        triangleText->setText("Triangles: %u", trianglesLastFrame);

        win.Display();
    }

    return 0;
}
