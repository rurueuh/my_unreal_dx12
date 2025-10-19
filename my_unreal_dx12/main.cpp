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
	srand(static_cast<unsigned int>(time(nullptr)));
    std::vector<Mesh> cubes;
	int numCubes = 100;
	Texture tex;
    Texture dirt;
	tex.LoadFromFile(win.GetGraphicsDevice(), "cup.jpg");
	dirt.LoadFromFile(win.GetGraphicsDevice(), "dirt.jpg");

	Mesh cubeMesh("teapot.txt");
	cubeMesh.SetPosition(30.0f, 0.0f, 0.0f);
	cubeMesh.SetTexture(&dirt);
	cubeMesh.setColor(1.0f, 1.0f, 0.0f);
    while (numCubes--) {
		Mesh cube("cube.txt");
        cube.SetPosition(((rand() % 100) / 100.f - 0.5f) * 50.f,
                         ((rand() % 100) / 100.f - 0.5f) * 50.f,
			((rand() % 100) / 100.f - 0.5f) * 50.f);
        cube.setColor((rand() % 100) / 100.f,
                      (rand() % 100) / 100.f,
			          (rand() % 100) / 100.f);
		cubes.push_back(cube);
    }

	Texture fighterTex;
	fighterTex.LoadFromFile(win.GetGraphicsDevice(), "fighter_jet.jpg");
	Mesh fighterJet("fighter_jet.obj");
	fighterJet.SetTexture(&fighterTex);
	fighterJet.SetPosition(0.0f, -10.0f, 20.0f);
	fighterJet.SetScale(0.1f, 0.1f, 0.1f);

	std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();

    while (win.IsOpen())
    {
        win.Clear();
        win.Draw(cubeMesh);
		win.Draw(fighterJet);
        for (auto& c : cubes) {
            win.Draw(c);
        }
        if (GetAsyncKeyState('P') & 0x0001)
        {
            static bool wireframe = false;
            wireframe = !wireframe;
            win.setWireframe(wireframe);
		}

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
        win.Display();

    }
}
