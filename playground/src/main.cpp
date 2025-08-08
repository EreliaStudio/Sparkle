#include <sparkle.hpp>

int main()
{
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"Playground", {{0, 0}, {800, 600}});
	window->setUpdateTimer(1);
	window->requestMousePlacement({400, 300});

	spk::SafePointer<spk::GameEngine> engine = new spk::GameEngine();
	spk::GameEngineWidget engineWidget(L"EngineWidget", window->widget());
	engineWidget.setGeometry({0, 0}, window->geometry().size);
	engineWidget.setGameEngine(engine);
	engineWidget.activate();

	spk::SafePointer<spk::Entity> player = new spk::Entity(L"Player");
	player->activate();
	engine->addEntity(player);

	auto cameraComponent = player->addComponent<spk::CameraComponent>(L"Player/CameraComponent");
	cameraComponent->setPerspective(60.0f, static_cast<float>(window->geometry().size.x) / static_cast<float>(window->geometry().size.y));
	auto playerController = player->addComponent<spk::FreeViewController>(L"Player/FreeViewController");
	player->transform().place({5.0f, 5.0f, 5.0f});
	player->transform().lookAt({0.0f, 0.0f, 0.0f});
	
	spk::SafePointer<spk::Entity> cube = new spk::Entity(L"Cube");
	cube->activate();
	engine->addEntity(cube);
	auto renderer = cube->addComponent<spk::ObjMeshRenderer>(L"Cube/ObjMeshRenderer");

	spk::Image texture = spk::Image("playground/resources/texture/CubeTexture.png");

	spk::ObjMesh cubeMesh;
	cubeMesh.loadFromFile("playground/resources/obj/cube.obj");
	renderer->setTexture(&texture);
	renderer->setMesh(&cubeMesh);
	cube->transform().place({0.0f, 0.0f, 0.0f});
	return app.run();
}
