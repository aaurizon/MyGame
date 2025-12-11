# Requirements

## Quick start (CMake)

1. Install the Vulkan SDK and update `VULKAN_SDK_ROOT` in `CMakeLists.txt` if it differs from the default `C:/VulkanSDK/1.4.335.0`.
2. Configure: `cmake -S . -B build`
3. Build: `cmake --build build`
4. Run: `./build/MyGame` (from the repo root). Use `WASD`/arrow keys to move the camera and the mouse to look around. Press `Esc` to close the window.

The engine targets Windows/Win32 today with OpenGL, Vulkan, and placeholder DirectX 11/12 backends; each render window (child or standalone) can pick its backend at runtime through `ARenderWindow::setGraphicsBackend`.

## MinGW

LLVCM ou MinGW-W64 (x86_64-15.2.0-release-posix-seh-ucrt-rt_v13-rev0)

x32 = i686
x64 = x86_64 (recommandé)

mcf   = thread plus performant que posix
posix = utilise winpthreads (recommandé)
win32 = windows official threads

msvcrt = legacy
ucrt   = nouveau standard (recommandé)

https://www.mingw-w64.org/downloads/

## Vulkan

https://vulkan.lunarg.com/sdk/home

Installer = C’est l’environnement complet destiné aux programmeurs.

Config = Simple fichier de configuration utilisé par l’installeur du SDK.
pas besoin si tu utilises l’installeur standard.
C’est uniquement utile pour des installations automatisées.

Runtime = C’est le composant que les joueurs doivent avoir pour exécuter des jeux Vulkan.
Il ne contient pas les outils et headers nécessaires pour développer.

RuntimeZip =Les mêmes runtimes que ci-dessus, mais sous forme zip (utiles pour une intégration manuelle). 

Prendre l'installer

### Vulkan SDK – Extra Components (Quick Summary)

**GLM Headers**  
Math library (vectors, matrices, quaternions) compatible with GLSL.  
Recommended for any 3D engine development.  
**Use: Yes**

**SDL Libraries and Headers**  
Window creation, input handling, and Vulkan surface support.  
Useful for cross-platform engines.  
Except if you use GLFW/Win32
**Use: Yes**

**Volk**  
Meta-loader that dynamically loads Vulkan function pointers.  
Simplifies initialization and improves portability.  
**Use: Yes**

**Vulkan Memory Allocator (VMA)**  
High-level GPU memory allocator.  
Greatly simplifies Vulkan memory management.  
**Use: Yes**

**Shader Toolchain Debug Symbols**  
Debug symbols for GLSL/SPIR-V tools.  
Not required for normal development.  
**Use: Optional**

**ARM64 Binaries for Cross-Compiling**  
Tools for building Vulkan applications targeting ARM64.  
Not needed when developing only for Windows x64.  
**Use: No (unless targeting ARM64)**


