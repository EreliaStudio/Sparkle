#include <sparkle.hpp>

class CameraComponent : public spk::Component
{
private:
	spk::Camera _camera;
	spk::ContractProvider::Contract _transformContract;

	void _updateUBO()
	{
		auto &cameraUBO = spk::Lumina::Shader::Constants::ubo(L"CameraUBO");
		const spk::Transform &transform = owner()->transform();
		cameraUBO.layout()[L"viewMatrix"] = transform.inverseModel();
		cameraUBO.layout()[L"projectionMatrix"] = _camera.projectionMatrix();
		cameraUBO.validate();

		float determinant = (_camera.projectionMatrix() * transform.inverseModel()).determinant();

		spk::cout << "Determinant = " << determinant << std::endl;
	}

public:
	CameraComponent(const std::wstring& p_name) :
		spk::Component(p_name)
	{
	}

	void setPerspective(float p_fov, float p_aspectRatio, float p_near = 0.1f, float p_far = 1000.0f)
	{
		_camera.setPerspective(p_fov, p_aspectRatio, p_near, p_far);
		_updateUBO();
	}

	void start() override
	{
		_transformContract = owner()->transform().addOnEditionCallback([this]() { _updateUBO(); });
		_updateUBO();
	}

	spk::Camera &camera()
	{
		return _camera;
	}
};

class PlayerControllerComponent : public spk::Component
{
private:
	float _moveSpeed;
	float _mouseSensitivity;

public:
	PlayerControllerComponent(const std::wstring& p_name, float p_moveSpeed = 5.0f, float p_mouseSensitivity = 0.1f) :
		spk::Component(p_name),
		_moveSpeed(p_moveSpeed),
		_mouseSensitivity(p_mouseSensitivity)
	{
	}

	void onUpdateEvent(spk::UpdateEvent &p_event) override
	{
		if ((p_event.keyboard == nullptr || p_event.mouse == nullptr) == true)
		{
			return;
		}

		spk::Vector3 movement(0.0f, 0.0f, 0.0f);

		if (((*p_event.keyboard)[spk::Keyboard::Key::Z] == spk::InputState::Down) == true)
		{
			movement += owner()->transform().up();
		}
		if (((*p_event.keyboard)[spk::Keyboard::Key::S] == spk::InputState::Down) == true)
		{
			movement -= owner()->transform().up();
		}
		if (((*p_event.keyboard)[spk::Keyboard::Key::Q] == spk::InputState::Down) == true)
		{
			movement -= owner()->transform().right();
		}
		if (((*p_event.keyboard)[spk::Keyboard::Key::D] == spk::InputState::Down) == true)
		{
			movement += owner()->transform().right();
		}

		if ((movement != spk::Vector3()) == true)
		{
			movement = movement.normalize();
			owner()->transform().move(movement * (float)p_event.deltaTime.seconds * _moveSpeed);
			p_event.requestPaint();
		}

		// spk::Vector2Int delta = p_event.mouse->deltaPosition;
		// if ((delta.x != 0 || delta.y != 0) == true)
		// {
		// 	owner()->transform().rotate({0.0f, -delta.y * _mouseSensitivity, -delta.x * _mouseSensitivity});
		// 	p_event.requestPaint();
		// }
	}
};

int main()
{
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"Playground", {{0, 0}, {800, 600}});

	spk::SafePointer<spk::GameEngine> engine = new spk::GameEngine();
	spk::GameEngineWidget engineWidget(L"EngineWidget", window->widget());
	engineWidget.setGeometry({0, 0}, window->geometry().size);
	engineWidget.setGameEngine(engine);
	engineWidget.activate();

	spk::SafePointer<spk::Entity> player = new spk::Entity(L"Player");
	player->activate();
	engine->addEntity(player);
	auto cameraComponent = player->addComponent<CameraComponent>(L"Player/CameraComponent");
	cameraComponent->setPerspective(60.0f, static_cast<float>(window->geometry().size.x) / static_cast<float>(window->geometry().size.y));
	player->addComponent<PlayerControllerComponent>(L"Player/PlayerControllerComponent", 5.0f, 0.1f);
	player->transform().place({0.0f, 0.0f, 3.0f});
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