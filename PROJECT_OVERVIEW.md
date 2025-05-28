# VulkanRendererBwiek

> A lightweight Vulkan rendering engine using PhysX GPU particles and basic scene management, written in C++17.

---

## 🏗️ Project Structure

| Folder        | Description                                    |
|:--------------|:-----------------------------------------------|
| `Engine/`      | Core engine modules (Renderer, Physics, Scenes, Meshes, GameObjects, Input, Windowing) |
| `vulkanbase/`  | Vulkan initialization, pipelines, sync objects, shader loading utilities |
| `shaders/`     | Vulkan GLSL shaders (compiled at build time) |
| `Resources/`   | 3D model files, assets |

---

## 📚 Main Components

### Vulkan Renderer Core
- **`RendererManager.h`**: Manages Vulkan pipelines, swapchains, and renders scenes each frame.
- **`Pipeline.h`**: Represents a Vulkan graphics pipeline (vertex/fragment shaders, depth, input layouts).
- **`ShaderBase.h`**: Loads SPIR-V shaders, manages descriptor sets and uniform buffers.
- **`VulkanBase.h`**: Full Vulkan setup (instance, device, swapchain, render passes, framebuffers).
- **`WindowManager.h`**: GLFW-based window management.

### Physics Simulation
- **`PhysxBase.h`**: Full NVIDIA PhysX GPU particle fluid simulation.
- **`PhysicsManager.h`**: Physics step and cleanup logic.

### Scene Management
- **`Scene.h`**: Abstract base class for scenes.
- **`MeshScene.h`**: Scene containing 3D mesh objects.
- **`ParticleScene.h`**: Scene containing particle groups.
- **`SceneModelManager.h`**: Unified management of Meshes and Particles.

### Meshes and Objects
- **`Mesh.h`**: Vulkan vertex/index buffer abstraction.
- **`BaseObject.h`**: Wraps a Mesh and exposes transformations.
- **`DataBuffer.h`**: Vulkan GPU buffer abstraction.
- **`CommandBuffer.h`**: Vulkan command buffer recording helper.
- **`CommandPool.h`**: Vulkan command pool manager.

### Game Object System
- **`GameObject.h`**: Simple ECS-style base object.
- **`Component.h`**: Components like `TransformComponent`, `ModelMeshComponent`, `PrimitiveMeshComponent`.

### Input and Camera
- **`InputManager.h`**: Keyboard and mouse input management using GLFW.
- **`Camera.h`**: First-person camera supporting movement and rotations.

---

## ⚙️ External Dependencies

| Library  | Source |
|:---------|:-------|
| **GLFW**  | Window + input handling (via vcpkg) |
| **GLM**   | Math library (vectors, matrices) (via vcpkg) |
| **Vulkan SDK** | Vulkan runtime and headers |
| **NVIDIA PhysX** | GPU particles and rigid body simulation (via vcpkg) |

---

## 🚀 Build Instructions

### Prerequisites
- Visual Studio 2022 (C++17)
- Vulkan SDK installed
- CUDA-compatible GPU
- Git and vcpkg installed (or auto-installed)

### Steps

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
