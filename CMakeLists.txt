cmake_minimum_required(VERSION 3.20)
project(MyGame)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Update the Vulkan SDK path if it differs on your machine.
set(VULKAN_SDK_ROOT "C:/VulkanSDK/1.4.335.0")

set(ENGINE_SOURCES
    src/ARenderWindow.cpp
    src/AViewport.cpp
    src/AWorld.cpp
    src/AEntity.cpp
    src/AFreeCamera.cpp
    src/Win32/AWindowImplWin32.cpp
    src/Graphics/OpenGL/OpenGLRenderer.cpp
    src/Graphics/Vulkan/VulkanRenderer.cpp
)

add_library(MyGameEngine STATIC ${ENGINE_SOURCES})
target_include_directories(MyGameEngine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${VULKAN_SDK_ROOT}/Include"
)
target_link_directories(MyGameEngine PRIVATE "${VULKAN_SDK_ROOT}/Lib")
target_link_libraries(MyGameEngine PUBLIC vulkan-1 opengl32 user32 gdi32)
target_compile_definitions(MyGameEngine PRIVATE
    VK_USE_PLATFORM_WIN32_KHR
    VMA_STATIC_VULKAN_FUNCTIONS=0
    VMA_DYNAMIC_VULKAN_FUNCTIONS=1
)

add_executable(MyGame src/main.cpp)
target_link_libraries(MyGame PRIVATE MyGameEngine)
