#include "sparkle.hpp"

int main()
{
    spk::Application app("Playground", spk::Vector2UInt(800, 800), spk::Application::Mode::Monothread);

    spk::SpriteSheet spriteSheet("Playground/resources/spriteSheet.png", spk::Vector2Int(4, 4));
    spk::GameEngine engine;

    spk::GameObject player("Player");
    player.transform().translation = spk::Vector3(0.0f, 0.0f, 1.0f);
    spk::MeshRenderer* renderer = player.addComponent<spk::MeshRenderer>("BodyRenderer");
    renderer->setSpriteSheet(&spriteSheet);
    renderer->setSprite(spk::Vector2Int(0, 0));

    spk::Camera camera("Camera");
    camera.transform().translation = spk::Vector3(0, 0, 10);
    camera.transform().lookAt(spk::Vector3(0, 0, 0));

    spk::GameObject obstacle("Obstacle");
    obstacle.transform().translation = spk::Vector3(2.5f, 0.0f, 1.0f);
    spk::MeshRenderer* renderer2 = obstacle.addComponent<spk::MeshRenderer>("BodyRenderer");
    renderer2->setSpriteSheet(&spriteSheet);
    renderer2->setSprite(spk::Vector2Int(0, 1));

    engine.subscribe(&player);
    engine.subscribe(&obstacle);
    engine.subscribe(&camera);

    spk::GameEngineManager gameEngineManager("GameEngineManager");
    gameEngineManager.setGameEngine(&engine);
    gameEngineManager.setGeometry(0, app.size());
    gameEngineManager.activate();

    return (app.run());
}
