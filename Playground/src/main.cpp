#include "sparkle.hpp"

class PlayerController : public spk::GameComponent
{
private:
    struct Input
    {
        spk::Keyboard::Key key;
        spk::Timer timer;
        std::function<void()> action;

        Input() :
            key(spk::Keyboard::A),
            timer(1),
            action(nullptr)
        {

        }

        Input(spk::Keyboard::Key p_key, size_t p_duration, std::function<void()> p_action) :
            key(p_key),
            timer(p_duration),
            action(p_action)
        {

        }

        void execute()
        {
            if (action == nullptr)
                return ;

            if (timer.isTimedOut() == true && spk::Application::activeApplication()->keyboard().getKey(key) == spk::InputState::Down)
            {
                action();
                timer.start();
            }
        }
    };

    spk::Physics* _physicsComponent;
    std::vector<Input> _inputs;

	void _onRender()
    {

    }

	void _onUpdate()
    {
        for (auto& input : _inputs)
        {
            input.execute();
        }
    }

    void _movePlayer(const spk::Vector3& p_velocityToAdd)
    {
        if (_physicsComponent == nullptr)
        {
            _physicsComponent = owner()->getComponent<spk::Physics>();
        }

        if (_physicsComponent != nullptr)
        {
            _physicsComponent->applyForce(p_velocityToAdd);
        }
    }

public:
    spk::GameObject* cameraObject;

    PlayerController(const std::string& p_name) :
        spk::GameComponent(p_name)
    {
        _physicsComponent = owner()->getComponent<spk::Physics>();
        _inputs.push_back(Input(spk::Keyboard::Z, 100, [&](){
                _movePlayer(spk::Vector3(0, 1.0f, 0));
            }));

        _inputs.push_back(Input(spk::Keyboard::S, 100, [&](){
                _movePlayer(spk::Vector3(0, -1.0f, 0));
            }));

        _inputs.push_back(Input(spk::Keyboard::Q, 100, [&](){
                _movePlayer(spk::Vector3(-1.0f, 0, 0));
            }));

        _inputs.push_back(Input(spk::Keyboard::D, 100, [&](){
                _movePlayer(spk::Vector3(1.0f, 0, 0));
            }));
    }
};

int main()
{
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(800, 800), spk::Application::Mode::Multithread);

    spk::SpriteSheet* entitySpriteSheet = new spk::SpriteSheet("entitySpriteSheet.png", spk::Vector2UInt(8, 4));
    spk::SpriteSheet* obstacleSpriteSheet = new spk::SpriteSheet("obstacleSpriteSheet.png", spk::Vector2UInt(4, 4));

    spk::GameEngine engine;
    //engine.addModule<spk::GravityModule>(spk::Vector3(0, -1.0f, 0));
    engine.addModule<spk::CollisionModule>();

    spk::GameObject playerObject("Player");
    engine.subscribe(&playerObject);
    playerObject.transform().translation = spk::Vector3(0, 0, 0);
    playerObject.transform().rotation = spk::Vector3(0, 0, 0);

    auto* playerSpriteRenderer = playerObject.addComponent<spk::SpriteRenderer>("Renderer");
    playerSpriteRenderer->setSpriteSheet(entitySpriteSheet);
    playerSpriteRenderer->setSprite(spk::Vector2Int(0, 0));

    auto* playerPhysics = playerObject.addComponent<spk::Physics>("Physics");
    playerPhysics->setKinematicState(true);
    auto* playerController = playerObject.addComponent<PlayerController>("PlayerController");
    auto* playerMeshCollider2D = playerObject.addComponent<spk::MeshCollider2D>("MeshCollider2D");
    playerMeshCollider2D->setMesh(playerSpriteRenderer->mesh());

    spk::GameObject cameraObject("Camera", &playerObject);
    playerController->cameraObject = &cameraObject;
    cameraObject.transform().translation = spk::Vector3(0, 0, -10);
    cameraObject.transform().lookAt(spk::Vector3(0, 0, 0));
    auto* cameraComponent = cameraObject.addComponent<spk::Camera>("MainCamera");
    cameraComponent->setAsMainCamera();
    cameraComponent->setType(spk::Camera::Type::Perspective);

    spk::GameObject obstacleObject("Obstacle");
    engine.subscribe(&obstacleObject);
    obstacleObject.transform().translation = spk::Vector3(0, -5, 0);
    obstacleObject.transform().scale = spk::Vector3(100, 1, 1);
    auto* obstacleSpriteRenderer = obstacleObject.addComponent<spk::SpriteRenderer>("Renderer");
    obstacleSpriteRenderer->setSpriteSheet(obstacleSpriteSheet);
    obstacleSpriteRenderer->setSprite(spk::Vector2Int(0, 0));
    auto* obstacleMeshCollider2D = obstacleObject.addComponent<spk::MeshCollider2D>("MeshCollider2D");
    obstacleMeshCollider2D->setMesh(obstacleSpriteRenderer->mesh());

    spk::GameEngineManager manager("GameEngine manager");
    manager.setGameEngine(&engine);
    manager.setGeometry(0, app.size());
    manager.activate();

    return (app.run());
}