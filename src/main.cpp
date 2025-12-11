#include <glm/glm.hpp>
#include <algorithm>
#include <AWindow>
#include <ARenderWindow>
#include <AViewport>
#include <AWorld>
#include <AEntity>
#include <AFreeCamera>
#include <AEvent>
#include <EEventKey>

int main(int argc, char* argv[])
{
    // Initialize: parent window + two child render windows (Vulkan on left, OpenGL on right).
    AWindow mainWindow("MyGame v1.0.0", 1200, 600);
    const int initialWidth = mainWindow.getWidth();
    const int initialHeight = mainWindow.getHeight();
    const int initialSplit = initialWidth / 2;

    ARenderWindow leftRender(mainWindow, 0, 0, initialSplit, initialHeight, EGraphicsBackend::Vulkan);
    ARenderWindow rightRender(mainWindow, initialSplit, 0, initialWidth - initialSplit, initialHeight, EGraphicsBackend::OpenGL);

    AViewport& viewportLeft = leftRender.getViewport();
    AViewport& viewportRight = rightRender.getViewport();

    // World
    AWorld world;
    viewportLeft.setWorld(world);
    viewportRight.setWorld(world);

    AEntity* e1 = AEntity::createTriangle(glm::vec3(0,0,0), glm::vec3(5, 0, 0), glm::vec3(0, 0, 5));
    AEntity* e2 = AEntity::createRectangle(20, 10);

    // Color setup: triangle with primary vertex colors (RGB), rectangle with sand-like color.
    e1->setVertexColors({
        {1.0f, 0.0f, 0.0f, 1.0f}, // Red
        {0.0f, 1.0f, 0.0f, 1.0f}, // Green
        {0.0f, 0.0f, 1.0f, 1.0f}  // Blue
    });
    e2->setColor({0.76f, 0.70f, 0.50f, 1.0f}); // Sand tone

    world.addEntity(e2);
    world.addEntity(e1);

    // Controls
    AFreeCamera camera(viewportLeft, glm::vec3(0, 0, 30), glm::vec3(0, 0, 0)); // Viewport, Position, Lookat
    camera.addViewport(viewportRight);
    bool cursorCaptured = true;
    mainWindow.setCursorGrabbed(cursorCaptured);
    camera.setInputEnabled(cursorCaptured);

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
        const int split = std::max(1, w / 2);
        leftRender.setRect(0, 0, split, h);
        rightRender.setRect(split, 0, w - split, h);
        return true;
    };
    if (updateViewportLayout()) {
        camera.refreshMatrices();
    }

    // Process
    while (mainWindow.isOpen())
    {
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
                    // Swap the backends between the two render windows for side-by-side comparison.
                    EGraphicsBackend leftBackend = leftRender.getGraphicsBackend();
                    EGraphicsBackend rightBackend = rightRender.getGraphicsBackend();
                    leftRender.setGraphicsBackend(rightBackend);
                    rightRender.setGraphicsBackend(leftBackend);
                }
            }

            camera.dispatchEvent(event);
        }

        leftRender.display();
        rightRender.display();
    }

    return 0;
}
