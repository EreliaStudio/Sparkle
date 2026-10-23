# Create an Entity and update the Engine

An Entity can be attached to an Engine and own Components or Behaviours. A System is an Engine-owned update participant.

~~~cpp
#include "core/context/update_context.hpp"
#include "engine/behaviour.hpp"
#include "engine/engine.hpp"
#include "engine/entity.hpp"
#include "engine/system.hpp"

#include <chrono>
#include <cstddef>

class CounterBehaviour : public spk::Behaviour
{
public:
    std::size_t count = 0;

protected:
    void _updateState(spk::UpdateContext &) override
    {
        ++count;
    }
};

class CounterSystem : public spk::System
{
public:
    std::size_t count = 0;

protected:
    void _updateState(spk::UpdateContext &) override
    {
        ++count;
    }
};

int main()
{
    spk::Engine engine;
    spk::Entity entity("Example");
    auto &behaviour = entity.addBehaviour<CounterBehaviour>();
    auto &system = engine.addSystem<CounterSystem>();
    engine.addEntity(&entity);

    spk::UpdateContext context{
        .time = std::chrono::seconds(1),
        .deltaTime = std::chrono::milliseconds(16)};
    engine.updateState(context);

    const bool updated = behaviour.count == 1 && system.count == 1;
    engine.removeEntity(&entity);
    return updated ? 0 : 1;
}
~~~

The Entity is a stack object, therefore `Engine::addEntity(&entity)` does not own it. The Engine owns the System; the Entity owns the Behaviour. This example follows the tested Core update contract in `tests/TU/Core/srcs/engine/engine_test.cpp`.

See [Entity](../reference/engine/Entity.md) and [Engine](../reference/engine/Engine.md).
