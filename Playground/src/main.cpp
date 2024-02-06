#include "playground.hpp"

int main()
{
	spk::Application app("Playground", spk::Vector2Int(600, 600), spk::Application::Mode::Monothread);
	app.setAsActiveApplication();

	return app.run();
}