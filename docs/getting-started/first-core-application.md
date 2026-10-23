# First Core application

This example uses the headless Engine; it does not create a window or an OpenGL context. It follows the behavior exercised by `tests/TU/Core/srcs/engine/engine_test.cpp`.

~~~cpp
#include "core/context/update_context.hpp"
#include "engine/engine.hpp"
#include "engine/entity3d.hpp"

#include <chrono>

int main()
{
    spk::Engine engine;
    spk::Entity3D player("Player");
    engine.addEntity(&player);

    spk::UpdateContext context{
        .time = std::chrono::seconds(1),
        .deltaTime = std::chrono::milliseconds(16)};
    engine.updateState(context);

    engine.removeEntity(&player);
}
~~~

`Engine::addEntity` attaches an existing Entity; it does not transfer ownership of the pointer. Keep the referenced Entity alive while it is attached. Systems created through `engine.addSystem<T>()` are owned by the Engine; components and behaviours created through `entity.addComponent<T>()` or `entity.addBehaviour<T>()` are owned by the Entity.

Learn more in [Engine](../reference/engine/Engine.md), [Entity](../reference/engine/Entity.md), and [creating an Entity](../guides/create-an-entity.md).
