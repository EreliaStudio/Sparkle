#include "sparkle.hpp"

enum class Event
{
    PlayerIdle,
    PlayerMotionUp,
    PlayerMotionLeft,
    PlayerMotionDown,
    PlayerMotionRight
};

class PlayerController : public spk::GameComponent
{
private:
    spk::NotifierContract contracts[5];

    spk::StateMachine<Event> _stateMachine;

    void _onUpdate()
    {
        _stateMachine.update();
    }

public:
    PlayerController() :
        spk::GameComponent("PlayerController"),
        contracts{
            spk::Application::activeApplication()->inputDecoder().subscribe(Event::PlayerIdle, [&](){ std::cout << "New event treated !" << std::endl; _stateMachine.enterState(Event::PlayerIdle);}),
            spk::Application::activeApplication()->inputDecoder().subscribe(Event::PlayerMotionUp, [&](){ std::cout << "New event treated !" << std::endl; _stateMachine.enterState(Event::PlayerMotionUp);}),
            spk::Application::activeApplication()->inputDecoder().subscribe(Event::PlayerMotionLeft, [&](){ std::cout << "New event treated !" << std::endl; _stateMachine.enterState(Event::PlayerMotionLeft);}),
            spk::Application::activeApplication()->inputDecoder().subscribe(Event::PlayerMotionDown, [&](){ std::cout << "New event treated !" << std::endl; _stateMachine.enterState(Event::PlayerMotionDown);}),
            spk::Application::activeApplication()->inputDecoder().subscribe(Event::PlayerMotionRight, [&](){ std::cout << "New event treated !" << std::endl; _stateMachine.enterState(Event::PlayerMotionRight);})
        }
    {
        _stateMachine.addState(Event::PlayerIdle, spk::StateMachine<Event>::Action(
            [&](){std::cout << "Entering Idle mode" << std::endl;},
            [&](){},
            [&](){std::cout << "Exiting Idle mode" << std::endl;}));
        _stateMachine.addState(Event::PlayerMotionUp, spk::StateMachine<Event>::Action(
            [&](){std::cout << "Entering MotionUp mode" << std::endl;}, 
            [&](){owner()->transform().translation += spk::Vector3(0, 1, 0);},
            [&](){std::cout << "Exiting MotionUp mode" << std::endl;}));
        _stateMachine.addState(Event::PlayerMotionLeft, spk::StateMachine<Event>::Action(
            [&](){std::cout << "Entering MotionLeft mode" << std::endl;},
            [&](){owner()->transform().translation += spk::Vector3(-1, 0, 0);},
            [&](){std::cout << "Exiting MotionLeft mode" << std::endl;}));
        _stateMachine.addState(Event::PlayerMotionDown, spk::StateMachine<Event>::Action(
            [&](){std::cout << "Entering MotionDown mode" << std::endl;},
            [&](){owner()->transform().translation += spk::Vector3(0, -1, 0);},
            [&](){std::cout << "Exiting MotionDown mode" << std::endl;}));
        _stateMachine.addState(Event::PlayerMotionRight, spk::StateMachine<Event>::Action(
            [&](){std::cout << "Entering MotionRight mode" << std::endl;},
            [&](){ owner()->transform().translation += spk::Vector3(1, 0, 0);},
            [&](){std::cout << "Exiting MotionRight mode" << std::endl;}));
    }
};

int main()
{
    spk::Application app("Playground", spk::Vector2UInt(800, 800), spk::Application::Mode::Monothread);

    app.inputDecoder().set_mappings({
            spk::InputMapping(spk::Input(spk::Keyboard::Z, spk::InputState::Pressed), Event::PlayerMotionUp),
            spk::InputMapping(spk::Input(spk::Keyboard::Q, spk::InputState::Pressed), Event::PlayerMotionLeft),
            spk::InputMapping(spk::Input(spk::Keyboard::S, spk::InputState::Pressed), Event::PlayerMotionDown),
            spk::InputMapping(spk::Input(spk::Keyboard::D, spk::InputState::Pressed), Event::PlayerMotionRight),

            spk::InputMapping(spk::Input(spk::Keyboard::Z, spk::InputState::Released), Event::PlayerIdle),
            spk::InputMapping(spk::Input(spk::Keyboard::Q, spk::InputState::Released), Event::PlayerIdle),
            spk::InputMapping(spk::Input(spk::Keyboard::S, spk::InputState::Released), Event::PlayerIdle),
            spk::InputMapping(spk::Input(spk::Keyboard::D, spk::InputState::Released), Event::PlayerIdle)
        });

    spk::SpriteSheet spriteSheet("Playground/resources/spriteSheet.png", spk::Vector2Int(4, 4));
    spk::GameEngine engine;

    spk::GameObject player("Player");
    player.addComponent<PlayerController>();
    spk::MeshRenderer* renderer = player.addComponent<spk::MeshRenderer>("BodyRenderer");
    renderer->setSpriteSheet(&spriteSheet);
    renderer->setSprite(spk::Vector2Int(0, 0));

    spk::Camera camera("Camera");
    player.addChild(&camera);
    camera.transform().translation = spk::Vector3(0, 0, 10);
    camera.transform().lookAt(spk::Vector3(0, 0, 0));

    spk::GameObject obstacle("Obstacle");
    obstacle.transform().translation = spk::Vector3(2.5f, 1.0258f, 1.0f);
    spk::MeshRenderer* renderer2 = obstacle.addComponent<spk::MeshRenderer>("BodyRenderer");
    renderer2->setSpriteSheet(&spriteSheet);
    renderer2->setSprite(spk::Vector2Int(0, 1));

    engine.subscribe(&player);
    engine.subscribe(&obstacle);

    spk::GameEngineManager gameEngineManager("GameEngineManager");
    gameEngineManager.setGameEngine(&engine);
    gameEngineManager.setGeometry(0, app.size());
    gameEngineManager.activate();

    return (app.run());
}