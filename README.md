# VulkanRenderer

A modular Vulkan-based rendering engine showcasing:

 
- **MeshScene**: Loadable meshes & geometric primitives (planes, cubes, rectangles) with per-vertex normals and colors.  
- **ParticleScene**: GPU-driven particle fluids via PhysX PBD & CUDA (bundled), rendered as point sprites.  
- **Component/GameObject System**: Simple ECS-style framework with `Transform`, `ModelMesh`, and `PrimitiveMesh` components.  
- **Unified SceneModelManager**: Single interface for adding/removing/updating mesh and particle objects.  
- **Fly-through Camera & Input**: WASD + mouse, built on GLFW (bundled).  
- **Multi-Pipeline Manager**: Two Vulkan pipelines (3D & particles) with depth buffering, descriptor sets, uniform buffers, and push-constants.  
- **VK_KHR_debug**: Validation layers and debug callbacks enabled in Debug builds.  

- **(In Construction: FractalScene)**: Real-time Mandelbrot/Julia fractal rendering via a fullscreen quad and push-constants. 

---

## Prerequisites

- **Vulkan SDK** (v1.3 or newer)  
  - Includes `glslc` for shader compilation, loader libraries, validation layers, headers, and `spirv-tools`.  
- **CMake** (v3.16 or newer)

> All other components (GLFW, GLM, PhysX, CUDA, etc.) are included or bundled in the repository—no extra installs required.

---

## Quick Start

1. **Clone repository**  
   ```bash
   git clone https://github.com/briek/VulkanRenderer.git
   cd VulkanRenderer
