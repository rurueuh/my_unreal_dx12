# My Unreal DX12 Renderer

This is a personal project demonstrating a DirectX 12 rendering engine. It is not affiliated with Unreal Engine.

## Features

*   **DirectX 12 API:** Utilizes the latest DirectX 12 API for high-performance graphics.
*   **Physically Based Rendering (PBR):** The renderer supports PBR materials, including albedo, normal, and metallic-roughness maps.
*   **Shadow Mapping:** Implements shadow mapping for realistic dynamic shadows.
*   **Imgui Integration:** Includes an ImGui layer for easy debugging and user interface creation.
*   **Mesh Loading:** Supports loading of `.obj` files.
*   **Primitive Shapes:** Includes functions to create primitive shapes like cubes, spheres, cylinders, and planes.

## Getting Started

### Prerequisites

*   Windows 10 or later
*   Visual Studio 2019 or later
*   Windows SDK (10.0.19041.0 or later)

### Building the Project

1.  Clone the repository:
    ```bash
    git clone https://github.com/rurueuh/Saovu.git
    ```
2.  Open the `my_unreal_dx12.sln` file in Visual Studio.
3.  Set the build configuration to `Debug` or `Release` and the platform to `x64`.
4.  Build the solution.

### Running the Project

After building the project, the executable will be located in the `x64/Debug` or `x64/Release` directory. Run `my_unreal_dx12.exe` to launch the application.

## Usage

The `main.cpp` file contains the main application logic. You can modify this file to add your own meshes, control the camera, and interact with the scene.

### Adding a Mesh

To add a mesh to the scene, create a `Mesh` object and pass it to the `WindowDX12::Draw` method in the main loop.

```cpp
// In main.cpp
// ...

// Load a mesh from a file
auto myMesh = std::make_shared<Mesh>("path/to/your/mesh.obj");
myMesh->SetPosition(0.0f, 0.0f, 0.0f);

// In the main loop
while (win.IsOpen())
{
    // ...
    win.Draw(*myMesh);
    // ...
}
```

### Controlling the Camera

The camera can be controlled using the keyboard and mouse. The default controls are:

*   **W, A, S, D, Q, E:** Move the camera
*   **Right Mouse Button + Mouse Movement:** Rotate the camera
*   **Shift:** Move faster

## Code Structure

The project is organized into the following main components:

*   **`main.cpp`:** The entry point of the application.
*   **`WindowDX12.h/.cpp`:** Manages the main window, DirectX 12 setup, and the main rendering loop.
*   **`Renderer.h/.cpp`:** Handles the rendering of meshes and manages the command list.
*   **`GraphicsDevice.h/.cpp`:** Encapsulates the D3D12 device and command queue.
*   **`ShaderPipeline.h/.cpp`:** Manages the creation of shader pipelines, including root signatures and PSOs.
*   **`Mesh.h/.cpp`:** Represents a 3D mesh and its transformations.
*   **`Camera.h/.cpp`:** Represents the camera in the 3D scene.
*   **`CameraController.h/.cpp`:** Handles camera movement and input.
