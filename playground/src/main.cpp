#include "playground.hpp"

enum class Shape
{
	Triangle,
	Square,
	Pentagon,
	Hexagon,
	Octagon,
	Circle
};

struct Transform
{
	spk::Vector2 position;
	spk::Vector2 scale;
	float rotation;
};

struct Body
{
	struct GPUView
	{
		spk::Vector2 offset;
	};

	GPUView gpuView() const
	{
		GPUView result;
		
		result.offset = transform.position;

		return (result);
	}

	Shape shape;
	Transform transform;
	spk::Color color;
};

struct Object
{
	using ID = long long;

	Body body;	

	Object()
	{

	}

	Object(const Shape& p_shape, const spk::Vector2& p_position, const spk::Color& p_color) :
		body{p_shape, {p_position, spk::Vector2(1, 1), 0}, p_color}
	{

	}

	Object(const Body& p_body) :
		body(p_body)
	{

	}
};

struct Entity : public Object
{
	Entity()
	{

	}

	Entity(const Shape& p_shape, const spk::Vector2& p_position, const spk::Color& p_color) :
		Object(p_shape, p_position, p_color)
	{

	}

	Entity(const Body& p_body) :
		Object(p_body)
	{

	}
};

struct Particule : public Object
{
	Particule()
	{

	}

	Particule(const Shape& p_shape, const spk::Vector2& p_position, const spk::Color& p_color) :
		Object(p_shape, p_position, p_color)
	{

	}

	Particule(const Body& p_body) :
		Object(p_body)
	{

	}
};

struct Projectile : public Object
{
	Projectile()
	{

	}

	Projectile(const Shape& p_shape, const spk::Vector2& p_position, const spk::Color& p_color) :
		Object(p_shape, p_position, p_color)
	{

	}

	Projectile(const Body& p_body) :
		Object(p_body)
	{

	}
};

struct PipelineAtlas : public spk::Singleton<PipelineAtlas>
{
	friend class spk::Singleton<PipelineAtlas>;

private:
	std::unordered_map<std::wstring, std::unique_ptr<spk::Pipeline>> _pipelines;

	std::unique_ptr<spk::Pipeline> _createBodyPipeline()
	{
		std::string vertexShaderCode = spk::FileUtils::readFileAsString(L"playground/data/resources/shader/object_renderer.vert");
		std::string fragmentShaderCode = spk::FileUtils::readFileAsString(L"playground/data/resources/shader/object_renderer.frag");

		std::unique_ptr<spk::Pipeline> result = std::make_unique<spk::Pipeline>(vertexShaderCode, fragmentShaderCode);

		result->addObjectLayoutAttribute(0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2);

		spk::OpenGL::UniformBufferObject cameraDataUBO(L"CameraData", 0, 128);
		cameraDataUBO.addElement(L"view", 0, sizeof(spk::Matrix4x4));
		cameraDataUBO.addElement(L"projection", 64, sizeof(spk::Matrix4x4));

		result->addUniformBufferObject(L"CameraData", std::move(cameraDataUBO));

		spk::OpenGL::ShaderStorageBufferObject bodyListSSBO(L"BodyList", 1, 0, 0, 8, 0);

		bodyListSSBO.dynamicArray().addElement(L"offset", 0, sizeof(spk::Vector2));

		// bodyListSSBO.dynamicArray().addElement(L"transform", 0, sizeof(Transform));
		// bodyListSSBO.dynamicArray().addElement(L"color", 32, sizeof(spk::Color));

		result->addObjectShaderStorageBufferObject(L"BodyList", std::move(bodyListSSBO));

		return (std::move(result)); 
	}

	PipelineAtlas()
	{
		_pipelines[L"objectRenderer"] = std::move(_createBodyPipeline());
	}

public:
	spk::SafePointer<spk::Pipeline> pipeline(const std::wstring& p_name)
	{
		if (_pipelines.contains(p_name) == false)
		{
			throw std::runtime_error("Pipeline [" + spk::StringUtils::wstringToString(p_name) + "] not found");
		}
		return (_pipelines[p_name].get());
	}
};

template<typename TStorageType>
class UnorderedGUIDMap
{
public:
    struct AllocationResult
    {
        spk::SafePointer<TStorageType> object;
        Object::ID id;
    };
private:
    Object::ID _nextId = 0;

    std::unordered_map<Object::ID, std::unique_ptr<TStorageType>> _storage;

public:
    Object::ID nextId()
    {
        return _nextId++;
    }

    AllocationResult add(TStorageType&& p_object)
    {
        Object::ID newId = nextId();
		
        spk::SafePointer<TStorageType> addedPtr = add(newId, std::move(p_object));

        return { addedPtr, newId };
    }

    spk::SafePointer<TStorageType> add(Object::ID p_id, TStorageType&& p_object)
    {
        if (_storage.contains(p_id))
		{
            return nullptr;
		}

        auto ptr = std::make_unique<TStorageType>(std::move(p_object));

        auto [it, inserted] = _storage.emplace(p_id, std::move(ptr));
        if (!inserted)
		{
            return nullptr;
		}

        return it->second.get();
    }

    spk::SafePointer<TStorageType> get(Object::ID p_id) const
    {
        auto it = _storage.find(p_id);
        if (it == _storage.end())
		{
            return nullptr;
		}

        return it->second.get();
    }

    bool remove(Object::ID p_id)
    {
        return _storage.erase(p_id) > 0;
    }

	auto begin() noexcept
    {
        return _storage.begin();
    }
    
    auto end() noexcept
    {
        return _storage.end();
    }
    
    auto begin() const noexcept
    {
        return _storage.begin();
    }
    
    auto end() const noexcept
    {
        return _storage.end();
    }

    auto cbegin() const noexcept
    {
        return _storage.cbegin();
    }
    
    auto cend() const noexcept
    {
        return _storage.cend();
    }
};

enum class Event
{
	OnObjectListEdition,
	OnPlayerPositionEdition,
	OnCameraProjectionEdition,
};

using EventDispatcher = spk::EventDispatcher<Event>;

struct Context : spk::Singleton<Context>
{
private:
	friend class spk::Singleton<Context>;
	EventDispatcher::Instanciator _eventDispatcherInstanciator;

	Context()
	{
		auto tmp = entities.add(Entity(Shape::Triangle, spk::Vector2(0, 0), spk::Color::blue));
		player = tmp.object;
		playerID = tmp.id;

		entities.add(Entity(Shape::Circle, spk::Vector2(-10, -4), spk::Color::red));
		entities.add(Entity(Shape::Pentagon, spk::Vector2(-4, -10), spk::Color::red));
		entities.add(Entity(Shape::Hexagon, spk::Vector2(-4, -4), spk::Color::red));
		entities.add(Entity(Shape::Octagon, spk::Vector2(-1, -1), spk::Color::red));
		entities.add(Entity(Shape::Triangle, spk::Vector2(1, 1), spk::Color::red));
		entities.add(Entity(Shape::Triangle, spk::Vector2(4, 4), spk::Color::red));
		entities.add(Entity(Shape::Triangle, spk::Vector2(4, 10), spk::Color::red));
		entities.add(Entity(Shape::Triangle, spk::Vector2(10, 4), spk::Color::red));

		camera.setOrthographic(-20, 20, -20, 20, 0, 10);
	}

public:
	UnorderedGUIDMap<Entity> entities;
	UnorderedGUIDMap<Particule> particules;
	UnorderedGUIDMap<Projectile> projectiles;

	spk::Camera camera;
	spk::SafePointer<Entity> player;
	Entity::ID playerID;
};

class ObjectRenderer
{
private:
	PipelineAtlas::Instanciator _pipelineAtlasInstanciator;

	spk::SafePointer<spk::Pipeline> _pipeline;

	spk::OpenGL::UniformBufferObject& _cameraUBO;

	spk::Pipeline::Object _triangleObjects;
	spk::OpenGL::ShaderStorageBufferObject& _triangleSSBO;

	spk::Pipeline::Object _squareObjects;
	spk::OpenGL::ShaderStorageBufferObject& _squareSSBO;

	spk::Pipeline::Object _pentagonObjects;
	spk::OpenGL::ShaderStorageBufferObject& _pentagonSSBO;

	spk::Pipeline::Object _hexagonObjects;
	spk::OpenGL::ShaderStorageBufferObject& _hexagonSSBO;

	spk::Pipeline::Object _octogonObjects;
	spk::OpenGL::ShaderStorageBufferObject& _octogonSSBO;

	spk::Pipeline::Object _circleObjects;
	spk::OpenGL::ShaderStorageBufferObject& _circleSSBO;

	void _feedVertexBufferObject(spk::OpenGL::BufferSet& p_bufferSetObject, float p_nbPoint)
	{
		int pointArraySize = static_cast<int>(std::ceil(p_nbPoint));

		std::vector<spk::Vector2> points;
		points.reserve(pointArraySize);

		float angleBetweenPoints = 360.0f / p_nbPoint;    

		for (int i = 0; i < pointArraySize; i++)
		{
			float angle = angleBetweenPoints * i;
			points.emplace_back(
					0.5f * std::cos(spk::degreeToRadian(angle)),
					0.5f * std::sin(spk::degreeToRadian(angle))
				);
		}

		std::vector<unsigned int> indexes;
		indexes.reserve(3 * (pointArraySize - 2));
		for (int i = 1; i < pointArraySize - 1; i++)
		{
			indexes.push_back(0);
			indexes.push_back(i);
			indexes.push_back(i + 1);
		}

		p_bufferSetObject.clear();
		p_bufferSetObject.layout()  << points;
		p_bufferSetObject.indexes() << indexes;
		p_bufferSetObject.validate();
	}

public:
	ObjectRenderer() :
		_pipeline(PipelineAtlas::instance()->pipeline(L"objectRenderer")),
		_cameraUBO(_pipeline->ubo(L"CameraData")),
		_triangleObjects(_pipeline->createObject()),
		_triangleSSBO(_triangleObjects.ssbo(L"BodyList")),
		_squareObjects(_pipeline->createObject()),
		_squareSSBO(_squareObjects.ssbo(L"BodyList")),
		_pentagonObjects(_pipeline->createObject()),
		_pentagonSSBO(_pentagonObjects.ssbo(L"BodyList")),
		_hexagonObjects(_pipeline->createObject()),
		_hexagonSSBO(_hexagonObjects.ssbo(L"BodyList")),
		_octogonObjects(_pipeline->createObject()),
		_octogonSSBO(_octogonObjects.ssbo(L"BodyList")),
		_circleObjects(_pipeline->createObject()),
		_circleSSBO(_circleObjects.ssbo(L"BodyList"))
	{
		_cameraUBO[L"view"] = spk::Matrix4x4::identity();
		_cameraUBO[L"projection"] = spk::Matrix4x4::identity();
		_cameraUBO.validate();

		_feedVertexBufferObject(_triangleObjects.bufferSet(), 3);
		_feedVertexBufferObject(_squareObjects.bufferSet(), 4);
		_feedVertexBufferObject(_pentagonObjects.bufferSet(), 5);
		_feedVertexBufferObject(_hexagonObjects.bufferSet(), 6);
		_feedVertexBufferObject(_octogonObjects.bufferSet(), 8);
		_feedVertexBufferObject(_circleObjects.bufferSet(), 20);
	}

	void clear()
	{
		_triangleSSBO.dynamicArray().clear();
		_squareSSBO.dynamicArray().clear();
		_pentagonSSBO.dynamicArray().clear();
		_hexagonSSBO.dynamicArray().clear();
		_octogonSSBO.dynamicArray().clear();
		_circleSSBO.dynamicArray().clear();
	}
	
	void prepareObject(const spk::SafePointer<Object>& p_object)
	{
		switch (p_object->body.shape)
		{
			case Shape::Triangle:
				_triangleSSBO.dynamicArray().push_back(p_object->body.gpuView());
				_triangleObjects.setNbInstance(_triangleSSBO.dynamicArray().nbElement());
				break;
			case Shape::Square:
				_squareSSBO.dynamicArray().push_back(p_object->body.gpuView());
				_squareObjects.setNbInstance(_squareSSBO.dynamicArray().nbElement());
				break;
			case Shape::Pentagon:
				_pentagonSSBO.dynamicArray().push_back(p_object->body.gpuView());
				_pentagonObjects.setNbInstance(_pentagonSSBO.dynamicArray().nbElement());
				break;
			case Shape::Hexagon:
				_hexagonSSBO.dynamicArray().push_back(p_object->body.gpuView());
				_hexagonObjects.setNbInstance(_hexagonSSBO.dynamicArray().nbElement());
				break;
			case Shape::Octagon:
				_octogonSSBO.dynamicArray().push_back(p_object->body.gpuView());
				_octogonObjects.setNbInstance(_octogonSSBO.dynamicArray().nbElement());
				break;
			case Shape::Circle:
				_circleSSBO.dynamicArray().push_back(p_object->body.gpuView());
				_circleObjects.setNbInstance(_circleSSBO.dynamicArray().nbElement());
				break;
		}
	}

	void validate()
	{
		_triangleSSBO.validate();
		_squareSSBO.validate();
		_pentagonSSBO.validate();
		_hexagonSSBO.validate();
		_octogonSSBO.validate();
		_circleSSBO.validate();
	}

	void setCameraView(spk::Matrix4x4 p_view)
	{
		// _cameraUBO[L"view"] = p_view;
		// _cameraUBO.validate();
	}

	void setCameraProjection(spk::Matrix4x4 p_projection)
	{
		_cameraUBO[L"projection"] = p_projection;
		_cameraUBO.validate();
	}

	void render()
	{
		_triangleObjects.render();
		_squareObjects.render();
		_pentagonObjects.render();
		_hexagonObjects.render();
		_octogonObjects.render();
		_circleObjects.render();
	}
};

class GameScreenWidget : public spk::Widget
{
private:
	Context::Instanciator _contextInstanciator;

	spk::EventDispatcher<Event>::Contract _onObjectListEditionContract;
	spk::EventDispatcher<Event>::Contract _onPlayerPositionEditionContract;
	spk::EventDispatcher<Event>::Contract _onCameraProjectionEditionContract;

	ObjectRenderer _objectRenderer;

	void _updateObjectRenderer()
	{
		_objectRenderer.clear();
		
		for (const auto& [id, entity] : Context::instance()->entities)
		{
			_objectRenderer.prepareObject(entity.get());
		}

		for (const auto& [id, particule] : Context::instance()->particules)
		{
			_objectRenderer.prepareObject(particule.get());
		}

		for (const auto& [id, projectile] : Context::instance()->projectiles)
		{
			_objectRenderer.prepareObject(projectile.get());
		}

		_objectRenderer.validate();
	}

	void _updateCameraView()
	{
		_objectRenderer.setCameraView(spk::Matrix4x4::translation(spk::Vector3(
				Context::instance()->player->body.transform.position, 0
			)));
	}

	void _updateCameraProjection()
	{
		_objectRenderer.setCameraProjection(Context::instance()->camera.projectionMatrix());
	}

	void _onPaintEvent(spk::PaintEvent& p_event) override
	{
		_objectRenderer.render();
	}

public:
	GameScreenWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_contextInstanciator()
	{
		_onObjectListEditionContract = EventDispatcher::instance()->subscribe(Event::OnObjectListEdition, [&]()
		{
			_updateObjectRenderer();
		});

		_onPlayerPositionEditionContract = EventDispatcher::instance()->subscribe(Event::OnPlayerPositionEdition, [&]()
		{
			_updateCameraView();
		});

		_onCameraProjectionEditionContract = EventDispatcher::instance()->subscribe(Event::OnCameraProjectionEdition, [&]()
		{
			_updateCameraProjection();
		});
	}
};

class MainWidget : public spk::Widget
{
private:
	Context::Instanciator _contextInstanciator;
	GameScreenWidget _gameScreenWidget;

	void _onGeometryChange() override
	{
		_gameScreenWidget.setGeometry(geometry());
	}

public:
	MainWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_widget) :
		spk::Widget(p_name, p_widget),
		_contextInstanciator(),
		_gameScreenWidget(p_name + L"/GameScreenWidget", this)
	{
		_gameScreenWidget.activate();
	}
};

int main()
{
	spk::GraphicalApplication app = spk::GraphicalApplication();

	spk::SafePointer<spk::Window> win = app.createWindow(L"TAAG", {{0, 0}, {800, 800}});
	win->setUpdateTimer(1);

	MainWidget mainWidget(L"MainWidget", win->widget());
	mainWidget.setGeometry(win->geometry());
	mainWidget.activate();
	
	EventDispatcher::instance()->emit(Event::OnObjectListEdition);
	EventDispatcher::instance()->emit(Event::OnPlayerPositionEdition);
	EventDispatcher::instance()->emit(Event::OnCameraProjectionEdition);

	return (app.run());
}