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

    world.addEntity(e1);
    world.addEntity(e2);

    // Controls
    AFreeCamera camera(viewport, glm::vec3(0, 0, 30), glm::vec3(0, 0, 0)); // Viewport, Position, Lookat

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
            }

            camera.dispatchEvent(event);
        }

        window.display();
    }

    return 0;
}
