#include <glm/glm.hpp>
#include <ARenderWindow>
#include <AViewport>
#include <AWorld>
#include <AEntity>
#include <AFreeCamera>
#include <AEvent>
#include <EEventKey>

int main(int argc, char* argv[])
{
    // Initialize
    ARenderWindow window("MyGame v1.0.0", 800, 600, EGraphicsBackend::Vulkan);
    AViewport& viewport = window.getViewport();

    // World
    AWorld world;
    viewport.setWorld(world);

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
    AFreeCamera camera(viewport, glm::vec3(0, 0, 30), glm::vec3(0, 0, 0)); // Viewport, Position, Lookat
    bool cursorCaptured = true;
    window.setCursorGrabbed(cursorCaptured);
    camera.setInputEnabled(cursorCaptured);

    // Process
    while (window.isOpen())
    {
        for (auto event : window.pollEvents())
        {
            if (event->is<AEvent::Closed>())
            {
                window.close();
            }
            else if (const auto* keyPressed = event->getIf<AEvent::KeyPressed>())
            {
                if (keyPressed->scancode == EEventKey::Scancode::Escape)
                    window.close();
                else if (keyPressed->scancode == EEventKey::Scancode::P)
                {
                    cursorCaptured = !cursorCaptured;
                    window.setCursorGrabbed(cursorCaptured);
                    camera.setInputEnabled(cursorCaptured);
                }
            }

            camera.dispatchEvent(event);
        }

        window.display();
    }

    return 0;
}
