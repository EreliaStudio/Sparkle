#include "sparkle.hpp"

void test()
{
    spk::Application app("Playground", spk::Vector2UInt(800, 800), spk::Application::Mode::Monothread);

    app.run();
}

int main()
{
    test();

    return 0;
}
