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

    std::vector<Vertex> verticesCube = {
        // Face avant (z = +1)
        {-1,-1, 1, 1,0,0, 0,1},
        {-1, 1, 1, 0,1,0, 0,0},
        { 1, 1, 1, 0,0,1, 1,0},
        { 1,-1, 1, 1,1,0, 1,1},

        // Face arrière (z = -1)
        { 1,-1,-1, 1,0,1, 0,1},
        { 1, 1,-1, 0,1,1, 0,0},
        {-1, 1,-1, 1,1,1, 1,0},
        {-1,-1,-1, 0.2f,0.7f,1, 1,1},

        // Face gauche (x = -1)
        {-1,-1,-1, 1,0,0, 0,1},
        {-1, 1,-1, 0,1,0, 0,0},
        {-1, 1, 1, 0,0,1, 1,0},
        {-1,-1, 1, 1,1,0, 1,1},

        // Face droite (x = +1)
        { 1,-1, 1, 1,0,1, 0,1},
        { 1, 1, 1, 0,1,1, 0,0},
        { 1, 1,-1, 1,1,1, 1,0},
        { 1,-1,-1, 0.2f,0.7f,1, 1,1},

        // Face haut (y = +1)
        {-1, 1, 1, 1,0,0, 0,1},
        {-1, 1,-1, 0,1,0, 0,0},
        { 1, 1,-1, 0,0,1, 1,0},
        { 1, 1, 1, 1,1,0, 1,1},

        // Face bas (y = -1)
        {-1,-1,-1, 1,0,1, 0,1},
        {-1,-1, 1, 0,1,1, 0,0},
        { 1,-1, 1, 1,1,1, 1,0},
        { 1,-1,-1, 0.2f,0.7f,1, 1,1},
    };

    std::vector<uint16_t> indicesCube = {
        0,1,2, 0,2,3,      // front
        4,5,6, 4,6,7,      // back
        8,9,10, 8,10,11,   // left
        12,13,14, 12,14,15,// right
        16,17,18, 16,18,19,// top
        20,21,22, 20,22,23 // bottom
    };


    std::vector<Mesh> cubes;
	int numCubes = 100;
	Texture tex;
	tex.LoadFromFile(win.GetGraphicsDevice(), "cup.jpg");
    Texture dirt;
	dirt.LoadFromFile(win.GetGraphicsDevice(), "dirt.jpg");
	Mesh cubeMesh;
	cubeMesh = cubeMesh.objLoader("teapot.txt");
	cubeMesh.m_tex = tex;
    while (numCubes--) {
		Mesh cube = cubeMesh;
		cube = cube.objLoader("cube.txt");
        cube.m_tex = dirt;
        //cube.Upload(win.GetDevice(), verticesCube, indicesCube);
        cube.SetPosition(((rand() % 100) / 100.f - 0.5f) * 50.f,
                         ((rand() % 100) / 100.f - 0.5f) * 50.f,
			((rand() % 100) / 100.f - 0.5f) * 50.f);
		cubes.push_back(cube);
    }

	std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();

    while (win.IsOpen())
    {
        win.Clear();
        win.Draw(cubeMesh);
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
