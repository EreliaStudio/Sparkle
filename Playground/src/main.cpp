#include "sparkle.hpp"

int main()
{
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(800, 800));

    return app.run();
}