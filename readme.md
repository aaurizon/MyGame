# ANNA Engine (AEngine)

AEngine is a modern, lightweight, and portable 3D graphics and game engine written in C++.  
It is designed with a strong focus on **KISS principles**, providing developers with a clean, intuitive, and highly efficient API for building games and real-time applications.

## Key Features

- **Object-oriented C++ architecture** with modern design practices.
- **Cross-platform support**:
    - Windows over Win32 – Fully supported
    - Linux over X11 – In development
    - macOS (planned)
- **Multiple graphics backends**:
    - OpenGL Core 3.2+
    - DirectX 11
    - DirectX 12
    - Vulkan
    - Metal (planned)
- **High performance**: Designed around low-level control, minimal overhead, and efficient rendering paths.
- **Simplicity first**: The API aims to be extremely easy to use while still providing access to advanced engine and rendering features.
- **Engine techniques inspired by modern game development**: clean abstractions, component-based logic (if applicable), optimized loops, and hardware-friendly design.

## Philosophy

AEngine follows two core ideas:

1. **Keep It Simple for Developers**  
   The engine exposes only what is necessary, avoiding complexity when not required. Clear naming, predictable workflows, and minimal boilerplate allow fast iteration and experimentation.

2. **Performance and Portability**  
   Internally, the engine uses robust and modern techniques to ensure performance across platforms and graphics APIs. The goal is to offer the same engine experience wherever it runs.

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| Windows  | ✔️ Supported | Win32 backend stable |
| Linux    | 🔧 In progress | X11 backend under development |
| macOS    | ⏳ Planned | Coming soon |

## Graphics API Support

| Graphics API | Status |
|--------------|--------|
| OpenGL Core (3.2+) | ✔️ Supported |
| DirectX 11 | ✔️ Supported |
| DirectX 12 | ✔️ Supported |
| Vulkan | ✔️ Supported |
| Metal | ⏳ Planned |

## Getting Started

### Requirements

- A C++17 (or later) compatible compiler
- CMake (recommended for project configuration)
- Platform SDKs depending on the selected backend:
    - Win32 SDK for Windows
    - X11 development packages for Linux
    - Graphics SDKs: DirectX, Vulkan SDK, etc.

### Building the Engine

```bash
mkdir build
cd build
cmake ..
cmake --build .
