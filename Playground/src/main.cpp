#include "sparkle.hpp"

int main()
{
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(800, 800));

    spk::SpriteSheet* entitySpriteSheet = new spk::SpriteSheet("entitySpriteSheet.png", spk::Vector2UInt(8, 4));
    spk::SpriteSheet* obstacleSpriteSheet = new spk::SpriteSheet("obstacleSpriteSheet.png", spk::Vector2UInt(4, 4));

    spk::GameEngine engine;

    spk::GameObject playerObject("Player");
    playerObject.transform().translation = spk::Vector3(0, 0, 0);
    auto* playerSpriteRenderer = playerObject.addComponent<spk::SpriteRenderer>("Renderer");
    playerSpriteRenderer->setSpriteSheet(entitySpriteSheet);
    playerSpriteRenderer->setSprite(spk::Vector2Int(0, 0));

    spk::GameObject cameraObject("Camera", &playerObject);
    cameraObject.transform().translation = spk::Vector3(0, 10, 0);
    cameraObject.transform().lookAt(spk::Vector3(0, 0, 0));
    auto* cameraComponent = cameraObject.addComponent<spk::Camera>("MainCamera");
    cameraComponent->setAsMainCamera();
    cameraComponent->setType(spk::Camera::Type::Perspective);

    spk::GameObject enemyObject("Enemy");
    enemyObject.transform().translation = spk::Vector3(3, 0, 3);
    auto* enemySpriteRenderer = enemyObject.addComponent<spk::SpriteRenderer>("Renderer");
    enemySpriteRenderer->setSpriteSheet(entitySpriteSheet);
    enemySpriteRenderer->setSprite(spk::Vector2Int(4, 0));

    spk::GameObject obstacleObject("Obstacle");
    obstacleObject.transform().translation = spk::Vector3(5, 0, 0);
    auto* obstacleSpriteRenderer = obstacleObject.addComponent<spk::SpriteRenderer>("Renderer");
    obstacleSpriteRenderer->setSpriteSheet(obstacleSpriteSheet);
    obstacleSpriteRenderer->setSprite(spk::Vector2Int(0, 0));

    engine.subscribe(&playerObject);
    engine.subscribe(&enemyObject);
    engine.subscribe(&obstacleObject);

    spk::GameEngineManager manager("GameEngine manager");
    manager.setGameEngine(&engine);
    manager.setGeometry(0, app.size());
    manager.activate();

    return app.run();
}