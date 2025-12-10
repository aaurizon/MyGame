cmake_minimum_required(VERSION 3.20)
project(MyGame)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(VULKAN_SDK_ROOT "C:/VulkanSDK/1.4.335.0")

set(SOURCES
    src/main.cpp
    src/Win32Window.cpp
    src/VulkanApp.cpp
)

add_executable(MyGame ${SOURCES})

target_include_directories(MyGame PRIVATE
    "${VULKAN_SDK_ROOT}/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_directories(MyGame PRIVATE "${VULKAN_SDK_ROOT}/Lib")
target_link_libraries(MyGame PRIVATE vulkan-1)
target_compile_definitions(MyGame PRIVATE
    VK_USE_PLATFORM_WIN32_KHR
    VMA_STATIC_VULKAN_FUNCTIONS=0
    VMA_DYNAMIC_VULKAN_FUNCTIONS=1
)
