cmake_minimum_required(VERSION 3.20)
project(MyGame)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Update the Vulkan SDK path if it differs on your machine.
set(VULKAN_SDK_ROOT "C:/VulkanSDK/1.4.335.0")

# Backend toggles
option(ENABLE_OPENGL "Build OpenGL backend" ON)
option(ENABLE_VULKAN "Build Vulkan backend" ON)
option(ENABLE_DX11 "Build DirectX11 backend" ON)
option(ENABLE_DX12 "Build DirectX12 backend" ON)

set(ENGINE_SOURCES
    src/AWindow.cpp
    src/AViewport.cpp
    src/AWorld.cpp
    src/AEntity.cpp
    src/AFreeCamera.cpp
    src/ARenderOverlay.cpp
    src/AFpsCounter.cpp
    src/ARenderTimeTracker.cpp
    src/Win32/AWindowImplWin32.cpp
)

if(ENABLE_OPENGL)
    list(APPEND ENGINE_SOURCES src/Graphics/OpenGL/OpenGLRenderer.cpp)
endif()

if(ENABLE_VULKAN)
    list(APPEND ENGINE_SOURCES src/Graphics/Vulkan/VulkanRenderer.cpp)
endif()

if(ENABLE_DX11)
    list(APPEND ENGINE_SOURCES src/Graphics/DirectX11/DirectX11Renderer.cpp)
endif()

if(ENABLE_DX12)
    list(APPEND ENGINE_SOURCES src/Graphics/DirectX12/DirectX12Renderer.cpp)
endif()

add_library(MyGameEngine STATIC ${ENGINE_SOURCES})
target_include_directories(MyGameEngine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

if(ENABLE_VULKAN)
    target_include_directories(MyGameEngine PUBLIC "${VULKAN_SDK_ROOT}/Include")
    target_link_directories(MyGameEngine PUBLIC "${VULKAN_SDK_ROOT}/Lib")
    target_compile_definitions(MyGameEngine PRIVATE
        VK_USE_PLATFORM_WIN32_KHR
        VMA_STATIC_VULKAN_FUNCTIONS=0
        VMA_DYNAMIC_VULKAN_FUNCTIONS=1
    )
endif()

target_link_libraries(MyGameEngine PUBLIC user32 gdi32 msimg32)

if(ENABLE_OPENGL)
    target_link_libraries(MyGameEngine PUBLIC opengl32)
endif()
if(ENABLE_VULKAN)
    target_link_libraries(MyGameEngine PUBLIC vulkan-1)
endif()
if(ENABLE_DX11)
    target_link_libraries(MyGameEngine PUBLIC d3d11 dxgi)
endif()
if(ENABLE_DX12)
    target_link_libraries(MyGameEngine PUBLIC d3d12 dxguid)
endif()

add_executable(MyGame src/main.cpp)
target_link_libraries(MyGame PRIVATE MyGameEngine)
