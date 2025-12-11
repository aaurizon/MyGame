#include <glm/glm.hpp>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>
#include <AWindow>
#include <AViewport>
#include <AWorld>
#include <AEntity>
#include <AFreeCamera>
#include <AEvent>
#include <AText>
#include <AFloatingText>
#include <ARenderOverlay>
#include <EEventKey>

int main(int argc, char* argv[])
{
    // Initialize: parent window + four child render windows (GL, VK, DX11, DX12).
    AWindow mainWindow("MyGame v1.0.0", 1200, 800);
    const int initialWidth = mainWindow.getWidth();
    const int initialHeight = mainWindow.getHeight();
    const int halfWidth = initialWidth / 2;
    const int halfHeight = initialHeight / 2;

    // 4 render windows: top-left OpenGL, top-right Vulkan, bottom-left DX11, bottom-right DX12.
    AWindow glRender(mainWindow, 0, 0, halfWidth, halfHeight, EGraphicsBackend::OpenGL);
    AWindow vkRender(mainWindow, halfWidth, 0, initialWidth - halfWidth, halfHeight, EGraphicsBackend::Vulkan);
    AWindow dx11Render(mainWindow, 0, halfHeight, halfWidth, initialHeight - halfHeight, EGraphicsBackend::DirectX11);
    AWindow dx12Render(mainWindow, halfWidth, halfHeight, initialWidth - halfWidth, initialHeight - halfHeight, EGraphicsBackend::DirectX12);

    AViewport& viewportGL = glRender.getViewport();
    AViewport& viewportVK = vkRender.getViewport();
    AViewport& viewportDX11 = dx11Render.getViewport();
    AViewport& viewportDX12 = dx12Render.getViewport();

    // World
    AWorld world;
    viewportGL.setWorld(world);
    viewportVK.setWorld(world);
    viewportDX11.setWorld(world);
    viewportDX12.setWorld(world);

    AEntity* e1 = AEntity::createTriangle(glm::vec3(0,0,0), glm::vec3(5, 0, 0), glm::vec3(0, 0, 5));
    AEntity* e2 = AEntity::createRectangle(20, 10);

    // Color setup: triangle with primary vertex colors (RGB), rectangle with sand-like color.
    e1->setVertexColors({
        {1.0f, 0.0f, 0.0f, 1.0f}, // Red
        {0.0f, 1.0f, 0.0f, 1.0f}, // Green
        {0.0f, 0.0f, 1.0f, 1.0f}  // Blue
    });
    e2->setColor({0.76f, 0.70f, 0.50f, 1.0f}); // Sand tone

    world.addEntity(e1);
    world.addEntity(e2);

    // Controls
    AFreeCamera camera(viewportGL, glm::vec3(0, 0, 30), glm::vec3(0, 0, 0)); // Viewport, Position, Lookat
    camera.addViewport(viewportVK);
    camera.addViewport(viewportDX11);
    camera.addViewport(viewportDX12);
    bool cursorCaptured = true;
    mainWindow.setCursorGrabbed(cursorCaptured);
    camera.setInputEnabled(cursorCaptured);

    // Overlays and floating texts.
    ARenderOverlay hudOverlay;
    AText& fpsText = hudOverlay.addText(AText("FPS: 0.0", {12, 12}, true, 16, {0.9f, 0.9f, 0.9f, 1.0f}));

    const auto& triVerts = e1->getVertices();
    glm::vec3 triCentroid{0.0f};
    for (const auto& v : triVerts) {
        triCentroid += v;
    }
    if (!triVerts.empty()) {
        triCentroid /= static_cast<float>(triVerts.size());
    }
    const glm::vec3 triLabelPos = triCentroid + glm::vec3(0.0f, 1.5f, 0.0f);
    AFloatingText* triangleLabel = new AFloatingText("Hello world!", triLabelPos, 18);
    world.addFloatingText(triangleLabel);

    viewportGL.addOverlay(hudOverlay);
    viewportVK.addOverlay(hudOverlay);
    viewportDX11.addOverlay(hudOverlay);
    viewportDX12.addOverlay(hudOverlay);

    using Clock = std::chrono::steady_clock;
    auto lastFrameTime = Clock::now();
    float accumulatedTime = 0.0f;
    int frames = 0;
    float fps = 0.0f;

    int lastLayoutWidth = -1;
    int lastLayoutHeight = -1;
    auto updateViewportLayout = [&]() -> bool {
        const int w = mainWindow.getWidth();
        const int h = mainWindow.getHeight();
        if (w == lastLayoutWidth && h == lastLayoutHeight) {
            return false;
        }
        lastLayoutWidth = w;
        lastLayoutHeight = h;
        const int halfW = std::max(1, w / 2);
        const int halfH = std::max(1, h / 2);
        glRender.setRect(0, 0, halfW, halfH);
        vkRender.setRect(halfW, 0, w - halfW, halfH);
        dx11Render.setRect(0, halfH, halfW, h - halfH);
        dx12Render.setRect(halfW, halfH, w - halfW, h - halfH);
        return true;
    };
    if (updateViewportLayout()) {
        camera.refreshMatrices();
    }

    // Process
    while (mainWindow.isOpen())
    {
        const auto now = Clock::now();
        const float deltaTime = std::chrono::duration<float>(now - lastFrameTime).count();
        lastFrameTime = now;
        accumulatedTime += deltaTime;
        ++frames;
        if (accumulatedTime >= 1.0f) {
            fps = static_cast<float>(frames) / accumulatedTime;
            frames = 0;
            accumulatedTime = 0.0f;
        }

        if (updateViewportLayout()) {
            camera.refreshMatrices();
        }

        for (auto event : mainWindow.pollEvents())
        {
            if (event->is<AEvent::Closed>())
            {
                mainWindow.close();
            }
            else if (const auto* keyPressed = event->getIf<AEvent::KeyPressed>())
            {
                if (keyPressed->scancode == EEventKey::Scancode::Escape)
                    mainWindow.close();
                else if (keyPressed->scancode == EEventKey::Scancode::P)
                {
                    cursorCaptured = !cursorCaptured;
                    mainWindow.setCursorGrabbed(cursorCaptured);
                    camera.setInputEnabled(cursorCaptured);
                }
                else if (keyPressed->scancode == EEventKey::Scancode::O)
                {
                    auto nextBackend = [](EGraphicsBackend b) {
                        switch (b) {
                        case EGraphicsBackend::OpenGL: return EGraphicsBackend::Vulkan;
                        case EGraphicsBackend::Vulkan: return EGraphicsBackend::DirectX11;
                        case EGraphicsBackend::DirectX11: return EGraphicsBackend::DirectX12;
                        case EGraphicsBackend::DirectX12: return EGraphicsBackend::OpenGL;
                        default: return EGraphicsBackend::OpenGL;
                        }
                    };
                    glRender.setGraphicsBackend(nextBackend(glRender.getGraphicsBackend()));
                    vkRender.setGraphicsBackend(nextBackend(vkRender.getGraphicsBackend()));
                    dx11Render.setGraphicsBackend(nextBackend(dx11Render.getGraphicsBackend()));
                    dx12Render.setGraphicsBackend(nextBackend(dx12Render.getGraphicsBackend()));
                }
            }

            camera.dispatchEvent(event);
        }

        char fpsBuffer[32];
        std::snprintf(fpsBuffer, sizeof(fpsBuffer), "FPS: %.1f", fps);
        fpsText.setText(fpsBuffer);

        glRender.display();
        vkRender.display();
        dx11Render.display();
        dx12Render.display();
    }

    return 0;
}
