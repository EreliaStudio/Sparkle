# Create a Publishable Part

Derive from `spk::Publishable::Part`, implement the two serialization hooks, and call `invalidate()` when the Source state changes.

~~~cpp
#include "design_pattern/publishable.hpp"

class HealthPart final : public spk::Publishable::Part
{
private:
    int _health = 100;

    void _encode(spk::ByteStream::Writer &writer) const override
    {
        writer << _health;
    }

    void _decode(const spk::ByteStream::Reader &reader) override
    {
        reader >> _health;
    }

public:
    void setHealth(int next)
    {
        _health = next;
        invalidate();
    }

    int health() const
    {
        return _health;
    }
};
~~~

A `Publishable`-derived owner registers the Part via the protected `setPartRegistry({{id, part, flags}})`. Keep the Part alive for the duration of its registration. For an Engine Component or Behaviour that is also a Part, use the provided `PublishableComponent` or `PublishableBehaviour` classes and override `_encodeData` / `_decodeData` rather than the final attachment wrappers.

Read [Publishable](../reference/multiplayer/Publishable.md) for initial/modified/all publication and capture/commit behavior, or see `tests/TU/Core/srcs/engine/publishable_entity_test.cpp` for an integrated usage example.
