#include <sparkle.hpp>

class CameraComponent : public spk::Component
{
private:
	spk::Camera _camera;
	spk::ContractProvider::Contract _transformContract;

	void _updateUBO()
	{
		auto &cameraUBO = spk::Lumina::ShaderObjectFactory::instance()->ubo(L"CameraUBO");
		const spk::Transform &transform = owner()->transform();
		cameraUBO.layout()[L"viewMatrix"] = transform.inverseModel();
		cameraUBO.layout()[L"projectionMatrix"] = _camera.projectionMatrix();
		cameraUBO.validate();
	}

public:
	CameraComponent() :
		spk::Component(L"CameraComponent")
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

class MeshRendererComponent : public spk::Component
{
private:
	spk::ObjMeshRenderer _renderer;
	spk::ObjMesh _mesh;
	spk::ContractProvider::Contract _transformContract;

	void _updateTransformUBO()
	{
		auto &transformUBO = spk::Lumina::ShaderObjectFactory::instance()->ubo(L"TransformUBO");
		transformUBO.layout()[L"modelMatrix"] = owner()->transform().model();
		transformUBO.validate();
	}

public:
	MeshRendererComponent() :
		spk::Component(L"MeshRendererComponent")
	{
	}

	void setMesh(const std::string &p_path)
	{
		_mesh.loadFromFile(p_path);
		_renderer.painter().clear();
		_renderer.painter().prepare(_mesh);
		_renderer.painter().validate();
	}

	void start() override
	{
		_transformContract = owner()->transform().addOnEditionCallback([this]() { _updateTransformUBO(); });
		_updateTransformUBO();
	}

	void onPaintEvent(spk::PaintEvent &p_event) override
	{
		_renderer.painter().render();
	}
};

class PlayerControllerComponent : public spk::Component
{
private:
	float _moveSpeed;
	float _mouseSensitivity;

public:
	PlayerControllerComponent(float p_moveSpeed = 5.0f, float p_mouseSensitivity = 0.1f) :
		spk::Component(L"PlayerControllerComponent"),
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
			movement += owner()->transform().forward();
		}
		if (((*p_event.keyboard)[spk::Keyboard::Key::S] == spk::InputState::Down) == true)
		{
			movement -= owner()->transform().forward();
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
		}

		spk::Vector2Int delta = p_event.mouse->deltaPosition;
		if ((delta.x != 0 || delta.y != 0) == true)
		{
			owner()->transform().rotate({0.0f, -delta.y * _mouseSensitivity, -delta.x * _mouseSensitivity});
		}
	}
};

int main()
{
	DEBUG_LINE();
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"Playground", {{0, 0}, {800, 600}});

	DEBUG_LINE();

	spk::SafePointer<spk::GameEngine> engine = new spk::GameEngine();
	spk::GameEngineWidget engineWidget(L"EngineWidget", window->widget());
	engineWidget.setGeometry({0, 0}, window->geometry().size);
	engineWidget.setGameEngine(engine);
	engineWidget.activate();

	DEBUG_LINE();
	spk::SafePointer<spk::Entity> player = new spk::Entity(L"Player");
	engine->addEntity(player);
	auto cameraComponent = player->addComponent<CameraComponent>();
	cameraComponent->setPerspective(60.0f, static_cast<float>(window->geometry().size.x) / static_cast<float>(window->geometry().size.y));
	player->addComponent<PlayerControllerComponent>();
	player->transform().place({0.0f, 0.0f, 3.0f});

	DEBUG_LINE();
	spk::SafePointer<spk::Entity> cube = new spk::Entity(L"Cube");
	engine->addEntity(cube);
	auto renderer = cube->addComponent<MeshRendererComponent>();
	renderer->setMesh("resources/obj/cube.obj");
	cube->transform().place({0.0f, 0.0f, 0.0f});

	DEBUG_LINE();
	return app.run();
}